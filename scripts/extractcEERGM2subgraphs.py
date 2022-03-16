#!/usr/bin/env python2
##############################################################################
#
# extractcERGM2subgraphs.py - get subgraphs for cERGM goodness-of-fit
#
# File:    extractcERGM2subgraphs.py
# Author:  Alex Stivala
# Created: March 2022
#
#
##############################################################################
"""Extract subgraphs for cERGM goodness-of-fit

 Input file is a Pajek format network file with either 1 edge per
 line or 1 node per line format (see
 https://snap.stanford.edu/snappy/doc/reference/LoadPajek.html)

 For each input graphs, writes new file with
 the induced subgraph induced by the union
 of the nodes in the last term (time period), and all nodes in
 earlier terms that receive arcs from them. 

 Output files are in cwd, with _cergm2_subgraph appended before suffix (.net)
 in Pajek network format. Also outputs files with term for each node similarly
 as _cergm2_terms.txt

 WARNING: the output files are overwritten if they exist.

 This is a reimplementation of the subgraph extract code from
 plotEstimNetDirectedcERGMSimFit2.R (R/igraph version) using Python and SNAP
 instead as R is too slow and not scalable.
 
 For SNAP see

 http://snap.stanford.edu/snappy/index.html

 Used version 4.1.0.

 NB using Python 2.7 as could net get SNAP to install in Python 3.

 Usage:
 
 python2 extractcERGM2subgraphs.py netfilename termfilename simNetFilePrefix

    netfilename is the Pajek format observed graph (the input arclistFile
       for EstimNetDirected)

    termfilename is the time periods (terms) of the nodes in the same
       format as any node attribute for EstimNetDirected. I.e. the first
       line is the attribute name (must be 'term' for this one) and each
       subsequent line is the value for a node, in order of the node
       numbering used in the arc list file (netfilename) i.e. first line
       after header has term (time period) for node 0, the next line for node
       1, and so on. The terms themsevles are numbered from 0 for the first
       term (time period). This is the same format used for terms in cERGM
       estimation in EstimNetDirected (and simulation in SimulateERGM); see
       add_Cergm_terms_to_digraph() in src/graph.c

    simNetFilePreifx is the prefix of the simulated network filenames
      this files have _x.net appended by EstimNetDirected or SimulateERGM,
      where is task or iteration number.
  

"""

import sys,os,time
import glob,re
import getopt
import gzip
import tempfile

import snap

from snowballSample import write_graph_file
from inducedSubgraphcERGM2 import cERGM2_subgraph


#-----------------------------------------------------------------------------
#
# Functions
#
#-----------------------------------------------------------------------------

def glob_re(pattern, strings):
    """
    Regular expression glob
    https://stackoverflow.com/questions/13031989/regular-expression-usage-in-glob-glob
    """
    return filter(re.compile(pattern).match, strings)



def convert_estimnetdirected_to_pajek_format(infile, outf):
    """
    Convert EstimNEtDirected Pajek network file format to the more verbose
    Pajek file format where each node is specified explicitly.
    EstimNetDirected uses a format like this:

    *Vertices 9032
    *Arcs
    641 1
    739 1
    ...

    where the nodes are (implicitly) numbered 1..N (9032 in the example above).
    This usually works with most software that uses Pajek format. 
    However not unfortunately with snap.LoadPajek() which requires the
    nodes all to be named, eg:

    *Vertices 9032
    1
    2
    ...
    9032
    *Arcs
    641 1
    739 1
    ...

    This function does that conversion.

    Parameters:
        infile  - filename of EstimNetDirected Pajek format file to read
        outf -  open file object of verbose Pajek format to write

    Return value:
        0 if ok else nonzero on error


    """
    dat = open(infile).readlines()
    if dat[0].split(' ')[0].lower() != "*vertices":   
        sys.stderr.write("Bad input file " + infile + " (no *vertices)\n")
        return  -1
    vertcount = int(dat[0].split(' ')[1])
    if dat[1].rstrip().lower() != "*arcs":
        sys.stderr.write("Bad input file " + infile + " (no *arcs)\n")
        return  -1
    nodenums = list(set([int(x) for x in flatten([y.rstrip().split(' ') for y in dat[2:]])]))
    if min(nodenums) < 1 or max(nodenums) > vertcount:
        sys.stderr.write("Bad input file " + infile + "(bad node number)\n")
    outf.write("*Vertices " + str(vertcount)+ "\n")
    outf.write("\n".join([str(x) for x in range(1, vertcount+1)]))
    outf.write("\n")
    outf.write("".join(dat[1:]))
    return 0


def flatten(t):
    """ flatten list of lists to a single list
    https://stackoverflow.com/questions/952914/how-to-make-a-flat-list-out-of-a-list-of-lists
    """
    return [item for sublist in t for item in sublist]

#-----------------------------------------------------------------------------
#
# main
#
#-----------------------------------------------------------------------------

def usage(progname):
    """
    print usage msg and exit
    """
    sys.stderr.write("usage: " + progname + "  netfilename termfilename simNEtFilePrefix\n")
    sys.exit(1)


def main():
    """
    See usage message in module header block
    """
    directed = False
    try:
        opts,args = getopt.getopt(sys.argv[1:], '')
    except:
        usage(sys.argv[0])
    for opt,arg in opts:
        usage(sys.argv[0])

    if len(args) != 3:
        usage(sys.argv[0])

    netfilename = args[0]
    termfilename = args[1]
    simnetfileprefix = args[2]

    tmpfd, tmpfile = tempfile.mkstemp()
    tmpf = os.fdopen(tmpfd, "w")
    if convert_estimnetdirected_to_pajek_format(netfilename, tmpf) != 0:
        sys.stderr.write('ERROR: bad data in ' + netfilename + '\n')
        sys.exit(1)
    tmpf.close()
    G = snap.LoadPajek(snap.PNGraph, tmpfile)
    os.remove(tmpfile)
    snap.PrintInfo(G)

    termdat = open(termfilename).readlines()
    if (termdat[0] != "term"):
        sys.stderr.write("ERROR: expecting 'term' as first line of " + termfilename + "\n")
        sys.exit(1)
    terms = [int(x) for x in termdat[1:]]
    print 'maxterm = ', max(terms)
    assert(len(terms) == G.GetNodes())
    for (i, node) in enumerate(g.Nodes()):
        G.AddIntAttrN(node, terms[i], "term")
    
    graph_glob = simnetfileprefix + "_[0-9]*[.]net"
    sim_files = glob_re(graph_glob, os.listdir())
    gz = False
    if len(sim_files) == 0:
        print 'No .net files with prefix ', simnetfileprefix, ' found, trying .net.gz instead'
        graph_glob = simnetfileprefix + "_[0-9]*[.]net.gz"
        sim_files = glob_re(graph_glob, os.listdir())
        gz = True
           

    for simfile in sim_files:
        if gz:
            tmpfd, tmpfile = tempfile.mkstemp()
            try:
                tmpfd.write(gzip.open(simfile).read())
                tmpfd.close()
                G = snap.LoadPajek(snap.PNGraph, tmpfile)
            finally:
                os.remove(tmpfile)
        else:
            G = snap.LoadPajek(snap.PNGraph, simfile)
        snap.PrintInfo(G)


if __name__ == "__main__":
    main()


