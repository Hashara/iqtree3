//
//  alisimulatorinvar.cpp
//  model
//
//  Created by Nhan Ly-Trong on 23/03/2021.
//

#include "alisimulatorinvar.h"

AliSimulatorInvar::AliSimulatorInvar(Params *params, double invar_prop) :
AliSimulator(params) {
    invariant_proportion = invar_prop;
}

AliSimulatorInvar::AliSimulatorInvar(AliSimulator *alisimulator, double invar_prop){
    tree = alisimulator->tree;
    params = alisimulator->params;
    num_sites_per_state = alisimulator->num_sites_per_state;
    length_ratio = alisimulator->length_ratio;
    expected_num_sites = alisimulator->expected_num_sites;
    partition_rate = alisimulator->partition_rate;
    invariant_proportion = invar_prop;
    max_length_taxa_name = alisimulator->max_length_taxa_name;
    fundi_items = alisimulator->fundi_items;
}

/**
*  simulate sequences for all nodes in the tree by DFS
*
*/
void AliSimulatorInvar::simulateSeqs(int sequence_length, double *site_specific_rates, ModelSubst *model, double *trans_matrix, int max_num_states, Node *node, Node *dad, ostream &out, vector<string> state_mapping)
{
    // process its neighbors/children
    NeighborVec::iterator it;
    FOR_NEIGHBOR(node, dad, it) {
        // reset the num_children_done_simulation
        if (node->num_children_done_simulation >= (node->neighbors.size() - 1))
            node->num_children_done_simulation = 0;
        
        // if a model is specify for the current branch -> simulate the sequence based on that branch-specific model
        if ((*it)->attributes["model"].length()>0)
            branchSpecificEvolution(sequence_length, trans_matrix, max_num_states, node, it);
        // otherwise, simulate the sequence based on the common model
        else
            simulateASequenceFromBranchAfterInitVariables(model, sequence_length, site_specific_rates, trans_matrix, max_num_states, node, it);
        
        // permuting selected sites for FunDi model
        if (params->alisim_fundi_taxon_set.size()>0)
        {
            if (node->isLeaf())
                permuteSelectedSites(fundi_items, node);
            if ((*it)->node->isLeaf())
                permuteSelectedSites(fundi_items, (*it)->node);
        }
        
        // writing and deleting simulated sequence immediately if possible
        writeAndDeleteSequenceImmediatelyIfPossible(out, state_mapping, it, node);
        
        // browse 1-step deeper to the neighbor node
        simulateSeqs(sequence_length, site_specific_rates, model, trans_matrix, max_num_states, (*it)->node, node, out, state_mapping);
    }
}


/**
*  simulate sequences for all nodes in the tree
*/
void AliSimulatorInvar::simulateSeqsForTree(string output_filepath)
{
    // get variables
    int sequence_length = expected_num_sites;
    double invariant_proportion = tree->getRate()->getPInvar();
    ModelSubst *model = tree->getModel();
    int max_num_states = tree->aln->getMaxNumStates();
    ostream *out;
    vector<string> state_mapping;
    
    // initialize the site-specific rates
    double *site_specific_rates = new double[sequence_length];
    initVariables(sequence_length, site_specific_rates);
    
    // initialize trans_matrix
    double *trans_matrix = new double[max_num_states*max_num_states];
    
    // write output to file (if output_filepath is specified)
    if (output_filepath.length() > 0)
    {
        try {
            // add ".phy" or ".fa" to the output_filepath
            if (params->aln_output_format != IN_FASTA)
                output_filepath = output_filepath + ".phy";
            else
                output_filepath = output_filepath + ".fa";
            if (params->do_compression)
                out = new ogzstream(output_filepath.c_str());
            else
                out = new ofstream(output_filepath.c_str());
            out->exceptions(ios::failbit | ios::badbit);

            // write the first line <#taxa> <length_of_sequence> (for PHYLIP output format)
            if (params->aln_output_format != IN_FASTA)
            {
                int num_leaves = tree->leafNum - ((tree->root->isLeaf() && tree->root->name == ROOT_NAME)?1:0);
                *out <<num_leaves<<" "<< round(expected_num_sites/length_ratio)*num_sites_per_state<< endl;
            }

            // initialize state_mapping (mapping from state to characters)
            initializeStateMapping(tree->aln, state_mapping);
        } catch (ios::failure) {
            outError(ERR_WRITE_OUTPUT, output_filepath);
        }
    }
    
    // simulate sequences with only Invariant sites option
    simulateSeqs(sequence_length, site_specific_rates, model, trans_matrix, max_num_states, tree->MTree::root, tree->MTree::root, *out, state_mapping);
    
    // close the file if neccessary
    if (output_filepath.length() > 0)
    {
        if (params->do_compression)
            ((ogzstream*)out)->close();
        else
            ((ofstream*)out)->close();
        delete out;
        
        // show the output file name
        cout << "An alignment has just been exported to "<<output_filepath<<endl;
    }

    // delete trans_matrix array
    delete[] trans_matrix;
    
    // delete the site-specific rates
    delete[] site_specific_rates;
    
    // removing constant states if it's necessary
    if (length_ratio > 1)
        removeConstantSites();
}

/**
    simulate a sequence for a node from a specific branch after all variables has been initializing
*/
void AliSimulatorInvar::simulateASequenceFromBranchAfterInitVariables(ModelSubst *model, int sequence_length, double *site_specific_rates, double *trans_matrix, int max_num_states, Node *node, NeighborVec::iterator it)
{
    // compute the transition probability matrix
    model->computeTransMatrix(partition_rate*(*it)->length, trans_matrix);
    
    // convert the probability matrix into an accumulated probability matrix
    convertProMatrixIntoAccumulatedProMatrix(trans_matrix, max_num_states, max_num_states);
    
    // estimate the sequence for the current neighbor
    (*it)->node->sequence.resize(sequence_length);
    for (int i = 0; i < sequence_length; i++)
    {
        
        // if this site is invariant -> preserve the dad's state
        if (site_specific_rates[i] == 0)
            (*it)->node->sequence[i] = node->sequence[i];
        else // otherwise, randomly select the state, considering it's dad states, and the transition_probability_matrix
        {
            int starting_index = node->sequence[i]*max_num_states;
            (*it)->node->sequence[i] = getRandomItemWithAccumulatedProbMatrixMaxProbFirst(trans_matrix, starting_index, max_num_states, node->sequence[i]);
        }
    }
}

/**
    initialize variables (e.g., site-specific rate)
*/
void AliSimulatorInvar::initVariables(int sequence_length, double *site_specific_rates)
{
    for (int i = 0; i < sequence_length; i++)
    {
        // if this site is invariant -> preserve the dad's state
        if (random_double() <= invariant_proportion)
            site_specific_rates[i] = 0;
        else
            site_specific_rates[i] = 1;
    }
}
