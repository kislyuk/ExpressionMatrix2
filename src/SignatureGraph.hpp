#ifndef CZI_EXPRESSION_MATRIX2_SIGNATURE_GRAPH_HPP
#define CZI_EXPRESSION_MATRIX2_SIGNATURE_GRAPH_HPP

// In the signature graph, each vertex represents
// the set of all cells with a given signature.

#include "BitSet.hpp"
#include "Ids.hpp"

#include <boost/graph/adjacency_list.hpp>

#include "iosfwd.hpp"
#include "map.hpp"



namespace ChanZuckerberg {
    namespace ExpressionMatrix2 {

        class SignatureGraph;
        class SignatureGraphEdge;
        class SignatureGraphVertex;

        // The base class for class SignatureGraph.
        typedef boost::adjacency_list<
            boost::vecS,
            boost::vecS,
            boost::undirectedS,
            SignatureGraphVertex,
            SignatureGraphEdge> SignatureGraphBaseClass;
    }
}


class ChanZuckerberg::ExpressionMatrix2::SignatureGraphVertex {
public:

    // The signature common to all cells of this vertex.
    BitSetPointer signature;

    // Number of cells with this signature.
    CellId cellCount;
};



class ChanZuckerberg::ExpressionMatrix2::SignatureGraphEdge {
public:
};



class ChanZuckerberg::ExpressionMatrix2::SignatureGraph :
    public SignatureGraphBaseClass {
public:

    // The vertex map gives the vertex with a given signature.
    map<BitSetPointer, vertex_descriptor> vertexMap;

    // Given the vertices, create the edges.
    void createEdges(size_t lshBitCount);

    // Write out the signature graph in Graphviz format.
    void write(const string& fileName) const;
    void write(ostream&) const;


private:
    class Writer {
    public:
        Writer(const SignatureGraph&);
        void operator()(ostream&) const;
        void operator()(ostream&, vertex_descriptor) const;
        void operator()(ostream&, edge_descriptor) const;
        const SignatureGraph& graph;
    };
};



#endif