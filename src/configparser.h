#ifndef CONFIGPARSER_H
#define CONFIGPARSER_H
/*****************************************************************************
 * 
 * File:    configparser.h
 * Author:  Alex Stivala, Maksym Byshkin
 * Created: October 2017
 *
 * Parse the configuration file to get algorithm paraemeters, input filenames,
 * parameters to estimate, etc.
 *
 * The config file is a text file with comments marked by '#'
 * character, and "keyword = value" pairs.  See config.txt for example
 * config file.
 *
 *
 ****************************************************************************/

#include <stdio.h>
#include "utils.h"
#include "changeStatisticsDirected.h"

typedef struct config_s {
  double ACA_S;           /* multiplier for step size in Algorithm S */
  double ACA_EE;          /* multiplier for step size in Algorithm EE */
  double compC;           /* multiplier of sd/mean theta to limit variance */
  uint_t samplerSteps;    /* sampler iterations per algorithm step */
  uint_t Ssteps;          /* steps of Algorithm S (adjusted by size) */
  uint_t EEsteps;         /* steps of Algorithm EE */
  uint_t EEinnerSteps;    /* inner iterations of Algorithm EE (adj. by size)*/
  bool   outputAllSteps;  /* write theta and dzA every iteration not just outer*/
  char *arclist_filename; /* filename of Pajek file with digraph to estimate */
  char *binattr_filename; /* filename of binary attributes file or NULL */
  char *catattr_filename; /* filename of categorical attributes file or NULL */
  char *contattr_filename;/* filename of continuous attributes file or NULL */
  char *theta_file_prefix;/* theta output filename prefix */
  char *dzA_file_prefix;  /* dzA output filename prefix */
  uint_t num_change_stats_funcs;           /* length of change_stats_funcs */
  change_stats_func_t **change_stats_funcs; /* structural parameter stats */
  const char          **param_names;        /* names corresponding to above */
  uint_t num_attr_change_stats_funcs;  /* length of attr_change_stats_funcs */
  attr_change_stats_func_t **attr_change_stats_funcs; /* attr param stats */
  char                     **attr_names; /* names of attributes for above */
  uint_t *attr_indices;   /* idx into digraph binattr/cattr/contattr for above */
  const char **attr_param_names; /* names corresponding to above two */
  uint_t num_dyadic_change_stats_funcs;  /* length of dyadic_change_stats_funcs */  
  dyadic_change_stats_func_t **dyadic_change_stats_funcs;/* dyadic change stats*/
  char                       **dyadic_names; /* names corresponding to above */
  uint_t *dyadic_indices;  /* idx into digraph binattr/cattr/contattr for above */
  const char **dyadic_param_names; /* names corresponding to above two */
} config_t;

void init_config_parser(void);
config_t *parse_config_file(const char *config_filename);
int build_attr_indices_from_names(config_t *config, const digraph_t *g);
int build_dyadic_indices_from_names(config_t *config, digraph_t *g);
void free_config_struct(config_t *config);

void dump_config_names(void);
void dump_parameter_names(void);

#endif /* CONFIGPARSER_H */

