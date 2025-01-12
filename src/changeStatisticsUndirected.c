/*****************************************************************************
 * 
 * File:    changeStatisticsUndirected.c
 * Author:  Alex Stivala
 * Created: January 2022
 *
 * Functions to compute graph change statistics for undirected graphs. Each
 * function takes a pointer to a graph struct, and two node numbers
 * i and j and returns the value of the change statistic for adding
 * the edge i -- j.
 *
 * Also takes lambda (decay) parameter which is only used for
 * some statistics ("alternating" statistics).
 *
 * For change statistics dependent on a nodal attribute, there is
 * an additional parameter a which is the index of the attribute
 * to use.
 *
 * On some functions there is additionally a parameter indicating when
 * the change statistic is being computed as part of a delete (rather
 * than add) move, which can be used for some implementations that can
 * be more easily implemented with this information. However in
 * general it is more elegant and simpler to compute the statistic for
 * adding the arc (for delete moves the value returned is just
 * negated, and the change statistic function does not depend on or
 * need to use this information at all).
 *
 * Some of these functions are adapted from the original PNet code by Peng Wang:
 *
 *   Wang P, Robins G, Pattison P. PNet: A program for the simulation and
 *   estimation of exponential random graph models. University of
 *   Melbourne. 2006.
 *
 * And for the definitions of the change statistics:
 * 
 *   Robins, G., Pattison, P., & Wang, P. (2009). Closure, connectivity and
 *   degree distributions: Exponential random graph (p*) models for
 *   directed social networks. Social Networks, 31(2), 105-117.
 * 
 *   Snijders, T. A., Pattison, P. E., Robins, G. L., & Handcock,
 *   M. S. (2006). New specifications for exponential random graph
 *   models. Sociological methodology, 36(1), 99-153.
 * 
 * And also generally:
 * 
 *   Lusher, D., Koskinen, J., & Robins, G. (Eds.). (2013). Exponential
 *   random graph models for social networks: Theory, methods, and
 *   applications. New York, NY: Cambridge University Press.
 * 
 * especially Ch. 6:
 *
 *   Koskinen, J., & Daraganova, G. (2013). Exponential random graph model
 *   fundamentals. In "Exponential random graph models for social networks:
 *   Theory, methods, and applications." (pp. 49-76). New York, NY:
 *   Cambridge University Press.
 *
 * As well as the statnet ergm terms, and references for specific
 * change statistics where indicated.
 *
 *
 * Do NOT compile with -ffast-math on gcc as we depend on IEEE handling of NaN
 *
 ****************************************************************************/

#include <math.h>
#include <assert.h>
#include "changeStatisticsUndirected.h"

/*****************************************************************************
 *
 * change statistics functions
 *
 ****************************************************************************/


/************************* Structural ****************************************/


/* 
 * Change statistic for Edge
 */
double changeEdge(graph_t *g, uint_t i, uint_t j, double lambda)
{
  (void)g; (void)i; (void)j; (void)lambda; /* unused parameters */
  assert(!g->is_directed);
  return 1;
}

/*
 * Change statistic for alternating k-stars (AS)
 */
double changeAltStars(graph_t *g, uint_t i, uint_t j, double lambda)
{
  assert(lambda > 1);
  assert(!g->is_directed);
  return lambda * (2 -
		   POW_LOOKUP(1-1/lambda, g->degree[i]) -
		   POW_LOOKUP(1-1/lambda, g->degree[j]));
}


/*
 * Change statistics for alternating two-path (A2P)
 */
double changeAltTwoPaths(graph_t *g, uint_t i, uint_t j, double lambda)
{
  uint_t v,k;
  double delta = 0;
  assert(lambda > 1);
  assert(!g->is_directed);

  if (i == j) {
    return 0;
  }

  for (k = 0; k < g->degree[j]; k++) {
    v = g->edgelist[j][k];
    if (v == i || v == j)
      continue;
    delta += POW_LOOKUP(1-1/lambda, GET_2PATH_ENTRY(g, i, v));
  }
  for (k = 0; k < g->degree[i]; k++) {
    v = g->edgelist[i][k];
    if (v == i || v == j)
      continue;
    delta += POW_LOOKUP(1-1/lambda, GET_2PATH_ENTRY(g, j, v));
  }

  return delta;
}


/*
 * Change statistic for alternating k-triangles (AT)
 */
double changeAltKTriangles(graph_t *g, uint_t i, uint_t j, double lambda)
{
  uint_t v,k,tmp;
  double  delta = 0;
  assert(lambda > 1);
  assert(!g->is_directed);
  
  if (i == j) {
    return 0;
  }
  if (g->degree[i] < g->degree[j]) {
    tmp = i;
    i = j;
    j = tmp;
  }

  for (k = 0; k < g->degree[j]; k++) {
    v = g->edgelist[j][k];
    if (v == i || v == j)
      continue;
    if (isEdge(g, i, v))
      delta += POW_LOOKUP(1-1/lambda, GET_2PATH_ENTRY(g, i, v)) +
	POW_LOOKUP(1-1/lambda, GET_2PATH_ENTRY(g, v, j));
  }
  delta += lambda * (1 - POW_LOOKUP(1-1/lambda, GET_2PATH_ENTRY(g, i, j)));
  return delta;
}


/************************* Actor attribute (binary) **************************/


/*
 * Change statistic for Activity
 */
double changeActivity(graph_t *g, uint_t i, uint_t j, uint_t a, bool isDelete)
{
  (void)isDelete; /*unused parameters*/
  assert(!g->is_directed);
  return ((g->binattr[a][i] == BIN_NA ? 0 : g->binattr[a][i]) +
	  (g->binattr[a][j] == BIN_NA ? 0 : g->binattr[a][j]));


}
