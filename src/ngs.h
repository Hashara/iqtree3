/***************************************************************************
 *   Copyright (C) 2009 by BUI Quang Minh   *
 *   minh.bui@univie.ac.at   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef NGS_H
#define NGS_H

#include "phylotree.h"
#include "alignmentpairwise.h"
#include "ratemeyerdiscrete.h"

/*
	collection of classes for Next-generation sequencing 
*/

/**
NGS Pairwise alignment

	@author BUI Quang Minh <minh.bui@univie.ac.at>
*/
class NGSAlignment : public AlignmentPairwise
{
public:

	/**
		constructor
		@param filename file in Fritz's format
	*/
    NGSAlignment(const char *filename);

	/**
		constructor
		@param nstate number of states
		@param ncat number of categories
		@param freq pair-state frequencies for all categories
	*/
	NGSAlignment(int nstate, int ncat, int *freq);

	/**
		read file in Fritz's format
	*/
	void readFritzFile(const char *filename);

	/**
		compute empirical state frequencies from the alignment
		@param state_freq (OUT) is filled with state frequencies, assuming state_freq was allocated with 
			at least num_states entries.
	*/
	virtual void computeStateFreq(double *state_freq);

	/**
		compute the sum of pair state frequencies over all categories
		@param sum_pair_freq (OUT) will be filled in with num_states*num_states entries. 
			Memory has to be allocated before calling this function.
	*/
	void computeSumPairFreq (int *sum_pair_freq);

	/**
		compute empirical rates between state pairs
		@param rates (OUT) vector of size num_states*(num_states-1)/2 for the rates
	*/
	virtual void computeEmpiricalRate (double *rates);

	/**
		compute the empirical distance for a category, used to initialize rate scaling factor
		@param cat specific category, between 0 and ncategory-1
	*/
	double computeEmpiricalDist(int cat);

	/**
		negative likelihood function for a category with a rate scaling factor
		@param cat specific category, between 0 and ncategory-1
		@param value a rate scaling factor
		@return negative log-likelihood (for minimization purpose)
	*/
	double computeFunctionCat(int cat, double value);

	/**
		negative likelihood and 1st and 2nd derivative function for a category with a rate scaling factor
		@param cat specific category, between 0 and ncategory-1
		@param value a rate scaling factor
		@param df (OUT) 1st derivative
		@param ddf (OUT) 2nd derivative
		@return negative log-likelihood (for minimization purpose)
	*/
	double computeFuncDervCat(int cat, double value, double &df, double &ddf);

	/**
		number of category
	*/
	int ncategory;
};


class NGSTree : public PhyloTree {

public:

    /**
     * Constructor with given alignment
     * @param params program parameters
     * @param alignment
     */
	NGSTree(Params &params, NGSAlignment *alignment);	

    /**
            compute the tree likelihood
            @param pattern_lh (OUT) if not NULL, the function will assign pattern log-likelihoods to this vector
                            assuming pattern_lh has the size of the number of patterns
            @return tree likelihood
     */
    virtual double computeLikelihood(double *pattern_lh = NULL);

    /**
            optimize all branch lengths of the tree
            @param iterations number of iterations to loop through all branches
            @return the likelihood of the tree
     */
    virtual double optimizeAllBranches(int iterations = 100);

};

class NGSRate : public RateMeyerDiscrete {
public:

	/**
		@param tree must be NGSTree type
	*/
	NGSRate(PhyloTree *tree);

	/**
		get rate category of a specified site-pattern. 
		@param ptn pattern ID 
		@return the rate category of the specified site-pattern
	*/
	virtual int getPtnCat(int ptn) { return 0; }

	/**
		optimize rates of all site-patterns
		compute categorized rates from the "continuous" rate of the original Meyer & von Haeseler model.
		The current implementation uses the k-means algorithm with k-means++ package.
	*/
	virtual double optimizeParameters();


	/**
		This function is inherited from Optimization class for optimizting site rates 
		@param value x-value of the function
		@return f(value) of function f you want to minimize
	*/
	virtual double computeFunction(double value);

	/**
		This function calculate f(value), first derivative f'(value) and 2nd derivative f''(value).
		@param value x-value of the function
		@param df (OUT) first derivative
		@param ddf (OUT) second derivative
		@return f(value) of function f you want to minimize
	*/
	virtual double computeFuncDerv(double value, double &df, double &ddf);

	/**
		classify rates into categories.
		@param tree_lh the current tree log-likelihood
	*/
	virtual double classifyRates(double tree_lh) { return tree_lh; }

	/**
		write information
		@param out output stream
	*/
	virtual void writeInfo(ostream &out);

};


/**
	Main function
	@param params input program parameters
*/
void runNGSAnalysis(Params &params);

#endif
