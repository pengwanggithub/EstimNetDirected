/*****************************************************************************
 * 
 * File:    simulation.c
 * Author:  Alex Stivala
 * Created: October 2019
 *
 * Draw samples from ERGM distribution of directed graphs.
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include "utils.h"
#include "digraph.h"
#include "basicSampler.h"
#include "ifdSampler.h"
#include "simulation.h"

/*
 * Generate digraphs from ERGM distribution with supplied parameters.
 *
 * Parameters:
 *   g      - (in/out) Initial digraph object (empty graph with N nodes
 *            intiially where N is number of nodes in graphs to simulate).
 *   n      - number of parameters (length of theta vector and total
 *            number of change statistics functions)
 *   n_attr - number of attribute change statistics functions
 *   n_dyadic -number of dyadic covariate change stats funcs
 *   n_attr_interaction - number of attribute interaction change stats funcs
 *   change_stats_funcs - array of pointers to change statistics functions
 *                        length is n - n_attr - n_dyadic - n_attr_interaction
 *   attr_change_stats_funcs - array of pointers to change statistics functions
 *                             length is n_attr
 *   dyadic_change_stats_funcs - array of pointers to dyadic change stats funcs
 *                             length is n_dyadic
 *   attr_interaction_change_stats_funcs - array of pointers to attribute
 *                           interaction (pair) change statistics functions.
 *                           length is n_attr_interaction.
 *   attr_indices   - array of n_attr attribute indices (index into g->binattr
 *                    or g->catattr) corresponding to attr_change_stats_funcs
 *                    E.g. for Sender effect on the first binary attribute,
 *                    attr_indices[x] = 0 and attr_change_stats_funcs[x] =
 *                    changeSender
 *   attr_interaction_pair_indices - array of n_attr_interaction pairs
 *                          of attribute inidices similar to above but
 *                          for attr_interaction_change_setats_funcs which
 *                          requires pairs of indices.
 *   sampler_m      - sampler iterations (per algorithm step)
 *   theta          - array of n parameter values corresponding to
 *                    change stats funcs. Allocated by caller.
 *                    iteration, not just every outer iteration.
 *   useIFDsampler  - if true, use the IFD sampler instead of the basic 
 *                    sampler
 *   ifd_K          - consant for multiplying IFD auxiliary parameter
 *                    (only used if useIFDsampler is True).
 *   useConditionalEstimation - if True, do conditional estimation of 
 *                              snowball network samples.
 *   forbidReciprocity - if True, constrain ERGM sampling so that reciprocated
 *                       arcs are not allowed to be created (so estimation
 *                       is conditional on no reciprocated arcs, should have
 *                       none in input observed graph).
 *   sim_net_file_prefix -  simulated network output filename prefix 
 *
 * Return value:
 *   Nonzero on error, 0 if OK.
 *
 */
int simulate_ergm(digraph_t *g, uint_t n, uint_t n_attr, uint_t n_dyadic,
                  uint_t n_attr_interaction,
                  change_stats_func_t *change_stats_funcs[],
                  attr_change_stats_func_t *attr_change_stats_funcs[],
                  dyadic_change_stats_func_t *dyadic_change_stats_funcs[],
                  attr_interaction_change_stats_func_t
                  *attr_interaction_change_stats_funcs[],
                  uint_t attr_indices[],
                  uint_pair_t attr_interaction_pair_indices[],
                  uint_t sampler_m,
                  double theta[],
                  bool useIFDsampler, double ifd_K,
                  bool useConditionalEstimation,
                  bool forbidReciprocity,
                  char *sim_net_file_prefix)
{
  FILE          *sim_outfile;
  char           sim_outfilename[PATH_MAX+1];
  double acceptance_rate;
  double *addChangeStats = (double *)safe_malloc(n*sizeof(double));
  double *delChangeStats = (double *)safe_malloc(n*sizeof(double));
  double dzArc; /* only used for IFD sampler */
  double ifd_aux_param = 0; /* auxiliary parameter for IFD sampler */

  printf("samplerSteps = %u\n", sampler_m);
  if (useIFDsampler)
    printf("IFD sampler ifd_K = %g\n", ifd_K);
  if (useConditionalEstimation)
    printf("Doing conditional simulation of snowball sample\n");
  if (forbidReciprocity)
    printf("Simulation is conditional on no reciprocated arcs\n");

  if (useIFDsampler) {
    acceptance_rate = ifdSampler(g, n, n_attr, n_dyadic, n_attr_interaction,
                                 change_stats_funcs, 
                                 attr_change_stats_funcs,
                                 dyadic_change_stats_funcs,
                                 attr_interaction_change_stats_funcs,
                                 attr_indices,
                                 attr_interaction_pair_indices,
                                 theta,
                                 addChangeStats, delChangeStats, sampler_m,
                                 TRUE, /*actually do moves */
                                 ifd_K, &dzArc, &ifd_aux_param,
                                 useConditionalEstimation,
                                 forbidReciprocity);
  } else {
    acceptance_rate = basicSampler(g, n, n_attr, n_dyadic,
                                   n_attr_interaction,
                                   change_stats_funcs, 
                                   attr_change_stats_funcs,
                                   dyadic_change_stats_funcs,
                                   attr_interaction_change_stats_funcs,
                                   attr_indices,
                                   attr_interaction_pair_indices,
                                   theta,
                                   addChangeStats, delChangeStats,
                                   sampler_m,
                                   TRUE,/*actually do moves*/
                                   useConditionalEstimation,
                                   forbidReciprocity);
  }


  fprintf(stderr, "acceptance rate = %g\n", acceptance_rate);/*XXX*/
  
  free(delChangeStats);
  free(addChangeStats);
  
  return 0;
}

/*
 * Do simulation process using basic or IFD sampler to draw samples
 * from ERGM digraph distribution.
 *
 * Parameters:
 *   config - (in/out)configuration settings structure  - this is modified
 *            by calling build_attr_indices_from_names() etc.
 *
 * Return value:
 *    0 if OK else -ve value for error.
 */
int do_simulation(sim_config_t * config)
{
  struct timeval start_timeval, end_timeval, elapsed_timeval;
  int            etime;
  digraph_t     *g;
  uint_t         n_struct, n_attr, n_dyadic, n_attr_interaction, num_param;
  double        *theta;
  uint_t         i, theta_i;
  
  g = allocate_digraph(config->numNodes);
  if (load_attributes(g, config->binattr_filename,
                      config->catattr_filename,
                      config->contattr_filename,
                      config->setattr_filename)) {
    fprintf(stderr, "ERROR: loading node attributes failed\n");
    return -1;
  }

  if (config->zone_filename) {
    if (add_snowball_zones_to_digraph(g, config->zone_filename)) {
      fprintf(stderr, "ERROR: reading snowball sampling zones from %s failed\n",
              config->zone_filename);
      return -1;
    }
#ifdef DEBUG_SNOWBALL
    dump_zone_info(g);
#endif /* DEBUG_SNOWBALL */
  }
  
  print_data_summary(g);
  print_zone_summary(g);

  /* now that we have attributes loaded in g, build the attr_indices
     array in the config struct */
  if (build_attr_indices_from_names(&config->param_config, g) != 0)  {
    fprintf(stderr, "ERROR in attribute parameters\n");
    return -1;
  }
  /* and similary for dyadic covariates */
  if (build_dyadic_indices_from_names(&config->param_config, g) != 0)  {
    fprintf(stderr, "ERROR in dyadic covariate parameters\n");
    return -1;
  }
  /* and attribute interaction parameters */
  if (build_attr_interaction_pair_indices_from_names(&config->param_config, g) != 0) {
    fprintf(stderr, "ERROR in attribute interaction parameters\n");
    return -1;
  }

  /* note num_param is computed here as build_dyadic_indices_from_names()
     can decrease config->num_dyadic_change_stats_funcs from its 
     initial value */
  n_struct = config->param_config.num_change_stats_funcs;
  n_attr = config->param_config.num_attr_change_stats_funcs;
  n_dyadic = config->param_config.num_dyadic_change_stats_funcs;
  n_attr_interaction = config->param_config.num_attr_interaction_change_stats_funcs;
  num_param =  n_struct + n_attr + n_dyadic + n_attr_interaction;
    
   /* Ensure that if conditional estimation is to be used, the snowball
      sampling zone structure was specified */
   if (config->useConditionalEstimation) {
     if (!config->zone_filename) {
       fprintf(stderr,
           "ERROR: conditional estimation requested but no zones specified\n");
       return -1;
     }
     if (g->max_zone < 1) {
       fprintf(stderr,
               "ERROR: conditional estimation requested but only one zone\n");
       return -1;
     }
   }

   /* TODO set theta values */
   theta = (double *)safe_calloc(num_param, sizeof(double));
   

   /* write parameters and their values to stdout */
   theta_i = 0;
   printf("\n");
   for (i = 0; i < config->param_config.num_change_stats_funcs; i++, theta_i++) 
     printf("%s = %g\n", config->param_config.param_names[i], theta[theta_i]);
   
   for (i = 0; i < config->param_config.num_attr_change_stats_funcs;
        i++, theta_i++) 
     printf("%s_%s = %g\n", config->param_config.attr_param_names[i],
            config->param_config.attr_names[i], theta[theta_i]);
   
   for (i = 0; i < config->param_config.num_dyadic_change_stats_funcs;
        i++, theta_i++)
     printf("%s = %g\n", config->param_config.dyadic_param_names[i],
       theta[theta_i]);
   
   for (i = 0; i < config->param_config.num_attr_interaction_change_stats_funcs;
        i++, theta_i++) 
     printf("%s_%s_%s = %g\n",
            config->param_config.attr_interaction_param_names[i],
            config->param_config.attr_interaction_pair_names[i].first,
            config->param_config.attr_interaction_pair_names[i].second,
            theta[theta_i]);
   
   
   printf("\nrunning simulation...\n");
   gettimeofday(&start_timeval, NULL);
   
   simulate_ergm(g, num_param, n_attr, n_dyadic, n_attr_interaction,
                 config->param_config.change_stats_funcs,
                 config->param_config.attr_change_stats_funcs,
                 config->param_config.dyadic_change_stats_funcs,
                 config->param_config.attr_interaction_change_stats_funcs,
                 config->param_config.attr_indices,
                 config->param_config.attr_interaction_pair_indices,
                 config->samplerSteps, 
                 theta,
                 config->useIFDsampler, config->ifd_K,
                 config->useConditionalEstimation,
                 config->forbidReciprocity,
                 config->sim_net_file_prefix);

   gettimeofday(&end_timeval, NULL);
   timeval_subtract(&elapsed_timeval, &end_timeval, &start_timeval);
   etime = 1000 * elapsed_timeval.tv_sec + elapsed_timeval.tv_usec/1000;
   printf("simulation took %.2f s\n", (double)etime/1000);

   free(theta);
   free_digraph(g);
   
  return 0;
}