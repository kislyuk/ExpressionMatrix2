// Class to describe an RNA expression matrix.

#ifndef CZI_EXPRESSION_MATRIX2_EXPRESSION_MATRIX_H
#define CZI_EXPRESSION_MATRIX2_EXPRESSION_MATRIX_H

// CZI.
#include "Cell.hpp"
#include "CellSets.hpp"
#include "GeneSet.hpp"
#include "HttpServer.hpp"
#include "Ids.hpp"
#include "MemoryMappedVector.hpp"
#include "MemoryMappedVectorOfLists.hpp"
#include "MemoryMappedVectorOfVectors.hpp"
#include "MemoryMappedStringTable.hpp"

// Boost libraries.
#include <boost/shared_ptr.hpp>

// Standard library, partially injected in the ExpressionMatrix2 namespace.
#include "map.hpp"
#include "string.hpp"
#include "utility.hpp"
#include <limits>

namespace ChanZuckerberg {
    namespace ExpressionMatrix2 {

    	class BitSet;
        class CellSimilarityGraph;
        class ExpressionMatrix;
        class ExpressionMatrixCreationParameters;
        class GraphInformation;
        class ServerParameters;

    }
}



// Class used to store various parameters that control the initial creation of
// the ExpressionMatrix.
class ChanZuckerberg::ExpressionMatrix2::ExpressionMatrixCreationParameters {
public:

    // The following parameters control the capacity of various hash tables
    // use to store strings.
    // These capacities are hard limits: after the capacity is reached,
    // inserting a new element triggers an endless loop
    // (because we use open addressing hash tables without rehashing and without checks).
    // For good performance of these hash tables, these capacities
    // should equal at least twice the actual expected number of strings
    // of each type that will be stored.
    uint64_t geneCapacity = 1<<18;              // Controls the maximum number of genes.
    uint64_t cellCapacity = 1<<24;              // Controls the maximum number of cells.
    uint64_t cellMetaDataNameCapacity = 1<<16;  // Controls the maximum number of distinct cell meta data name strings.
    uint64_t cellMetaDataValueCapacity = 1<<28; // Controls the maximum number of distinct cell meta data value strings.
};



// Class used to store information about a cell similarity graph.
class ChanZuckerberg::ExpressionMatrix2::GraphInformation {
public:
    string cellSetName;
    string similarPairsName;
    double similarityThreshold;
    size_t maxConnectivity;
    size_t vertexCount;
    size_t edgeCount;
    size_t isolatedVertexCount;		// The number of isolated vertices that were removed.
    GraphInformation() {}
};



// Class used to specify parameters when starting the http server.
class ChanZuckerberg::ExpressionMatrix2::ServerParameters {
public:
	uint16_t port = 17100;	// The port number to listen to.
	string docDirectory;	// The directory containing the documentation (optional).
};



class ChanZuckerberg::ExpressionMatrix2::ExpressionMatrix : public HttpServer {
public:


    // Construct a new expression matrix. All binary data for the new expression matrix
    // will be stored in the specified directory. If the directory does not exist,
    // it will be created. If the directory already exists, any previous
    // expression matrix stored in the directory will be overwritten by the new one.
    ExpressionMatrix(const string& directoryName, const ExpressionMatrixCreationParameters&);

    // Access a previously created expression matrix stored in the specified directory.
    ExpressionMatrix(const string& directoryName);

    // Add a gene.
    // Returns true if the gene was added, false if it was already present.
    bool addGene(const string& geneName);

    // Add a cell to the expression matrix.
    // The meta data is passed as a vector of names and values, which are all strings.
    // The cell name should be entered as meta data "CellName".
    // The expression counts for each gene are passed as a vector of pairs
    // (gene names, count).
    // Returns the id assigned to this cell.
    // This changes the metaData vector so the CellName entry is the first entry.
    // It also changes the expression counts - it sorts them by decreasing count.
    CellId addCell(
        vector< pair<string, string> >& metaData,
        vector< pair<string, float> >& expressionCounts,
        size_t maxTermCountForApproximateSimilarityComputation
        );

    // Version of addCell that takes JSON as input.
    // The expected JSON can be constructed using Python code modeled from the following:
    // import json
    // cell = {'metaData': {'cellName': 'abc', 'key1': 'value1'}, 'expressionCounts': {'gene1': 10,'gene2': 20}}
    // expressionMatrix.addCell(json.dumps(jSonString), maxTermCountForApproximateSimilarityComputation)
    // Note the cellName metaData entry is required.
    CellId addCell(
        const string&,
        size_t maxTermCountForApproximateSimilarityComputation);



    /*******************************************************************************

    Add cells from data in files with fields separated by commas or by other separators.
    A fields can contain separators, as long as the entire field is quoted.

    This requires two input files, one for expression counts and one for cell meta data.
    The separators for each file are specified as arguments to this function.

    The expression counts file has a row for each gene and a column for each cell.
    In addition, it has an additional header row, before all other rows, containing cell names,
    and an additional column, before all other columns, containing gene names.
    The entry in the first column of the first row is ignored but must be present (can be empty).

    The meta data file has a row for each cell and a colun for each meta data field name.
    In addition, it has an additional header row, before all other rows, containing meta data names,
    and an additional column, before all other columns, containing cell names.
    Again, the entry in the first column of the first row is ignored but must be present (can be empty).

    The cell names in the first row of the expression count file and in the first column of the meta data file
    don't have to be in the same order.

    If a cell name is present in only one of the files, that cell is ignored.

    An example of the two files follow:

    Expression counts file:
    Dontcare,Cell1,Cell2,Cell3
    Gene1,10,20,30
    Gene2,30,40,50

    Meta data file:
    Dontcare,Name1,Name2
    Cell1,abc,def
    Cell2,123,456
    Cell3,xyz,uv

     *******************************************************************************/
    void addCells(
        const string& expressionCountsFileName,
        const string& expressionCountsFileSeparators,
        const string& metaDataFileName,
        const string& metaDataFileSeparators,
        size_t maxTermCountForApproximateSimilarityComputation
        );



	/*******************************************************************************

	Add cells from an hdf5 file in the format typically created by the 10X Genomics
	pipeline.

	See here for an example of a typical hdf5 file that we want to process:
	http://cf.10xgenomics.com/samples/cell-exp/1.3.0/t_3k_4k_aggregate/t_3k_4k_aggregate_filtered_gene_bc_matrices_h5.h5

	You can use command hdf5ls (from package hdf5-tools) to list the contents of the file:

	h5ls -r t_3k_4k_aggregate_filtered_gene_bc_matrices_h5.h5
	/                        Group
	/GRCh38                  Group
	/GRCh38/barcodes         Dataset {8083}
	/GRCh38/data             Dataset {8863417/Inf}
	/GRCh38/gene_names       Dataset {33694}
	/GRCh38/genes            Dataset {33694}
	/GRCh38/indices          Dataset {8863417/Inf}
	/GRCh38/indptr           Dataset {8084/8192}
	/GRCh38/shape            Dataset {2/16384}

	Data sets data, indices, and indptr contain the expression matrix in sparse format, by cell.

	The gene names are in data set gene_names. Data set genes is not used. It contains alternative
	gene ids.

	Cell names (or rather the corresponding barcodes) are in data set barcodes.

	Data set shape is also not used.

	*******************************************************************************/
    void addCellsFromHdf5(
    	const string& fileName,
    	size_t maxTermCountForApproximateSimilarityComputation
    );



    // Add cells from files created by the BioHub pipeline.
    // See the beginning of ExpressionMatrixBioHub.cpp for a detailed description
    // of the expected formats.
    void addCellsFromBioHub(
		const string& expressionCountsFileName,	// The name of the csv file containing expression counts.
		size_t initialMetaDataCount,			// The number of initial columns containing meta data.
		size_t finalMetaDataCount,				// The number of final columns containing meta data.
		const string& plateMetaDataFileName,	// The name of the file containing per-plate meta data.
		size_t maxTermCountForApproximateSimilarityComputation
    	);
	void getPlateMetaDataFromBioHub(
		const string& plateName,
		const string&plateMetaDataFileName,
		vector< pair<string, string> >& plateMetaData);

	// Add cell meta data contained in a csv file, one line per cell.
	// This can be used to read cell mata data in the BioHub pipeline
	// stored in the .log-by-cell.csv files.
    void addCellMetaData(const string& cellMetaDataName);




    // Return the number of genes.
    GeneId geneCount() const
    {
        return GeneId(geneNames.size());
    }

    // Return the number of cells.
    CellId cellCount() const
    {
        return CellId(cellMetaData.size());
    }

    // Return the value of a specified meta data field for a given cell.
    // Returns an empty string if the cell does not have the specified meta data field.
    string getMetaData(CellId, const string& name) const;
    string getMetaData(CellId, StringId) const;

    // Set a meta data (name, value) pair for a given cell.
    // If the name already exists for that cell, the value is replaced.
    void setMetaData(CellId, const string& name, const string& value);
    void setMetaData(CellId, StringId nameId, const string& value);
    void setMetaData(CellId, StringId nameId, StringId valueId);

    // Compute a sorted histogram of a given meta data field.
    void histogramMetaData(
        const CellSets::CellSet& cellSet,
        StringId metaDataNameId,
        vector< pair<string, size_t> >& sortedHistogram) const;

    // Compute the similarity between two cells given their CellId.
    // The similarity is the correlation coefficient of their
    // expression counts.
    double computeCellSimilarity(CellId, CellId) const;

    // Approximate but fast computation of the similarity between two cells.
    double computeApproximateCellSimilarity(CellId, CellId) const;

    // Compute a histogram of the difference between approximate and exact similarity,
    // looping over all pairs. This is O(N**2) slow.
    void analyzeAllPairs() const;

    // Compute the average expression vector for a given set of cells.
    // The last parameter controls the normalization used for the expression counts
    // for averaging:
    // 0: no normalization (raw read counts).
    // 1: L1 normalization (fractional read counts).
    // 2: L2 normalization.
    void computeAverageExpression(
    	const vector<CellId> cells,
		vector<double>& averageExpression,
		size_t normalization) const;



    // Find similar cell pairs by looping over all pairs.
    // This can use an exact or approximate mode of operation, depending
    // on the value of the last argument.
    // In the exact mode of operation, all gene expression counts for both cells
    // (stored in cellExpressionCounts) are taken into account.
    // In the approximate mode of operation, only the largest gene expression counts for both cells
    // (stored in largeCellExpressionCounts) are taken into account.
    // The number of largest gene expression counts to be stored for each cell
    // was specified when each cell was added, as an argument to addCell or addCells.
    // Both the exact an the approximate mode of operation are O(N**2) slow
    // because they loop over cell pairs. However the approximate mode of operation
    // is usually much faster than the exact mode of operation
    // (typically 10 times faster or better when the number of stored
    // largest expression counts for each cell is 100.
    void findSimilarPairs0(
   		const string& cellSetName,	// The name of the cell set to be used.
        const string& name,         // The name of the SimilarPairs object to be created.
        size_t k,                   // The maximum number of similar pairs to be stored for each cell.
        double similarityThreshold, // The minimum similarity for a pair to be stored.
        bool useExactSimilarity     // Use exact of approximate cell similarity computation.
        );



    // Find similar cell pairs by looping over all pairs
    // and using an LSH approximation to compute the similarity between two cells.
    // See the beginning of ExpressionMatrixLsh.cpp for more information.
    // Like findSimilarPairs0, this is also O(N**2) slow. However
    // the coefficient of the N**2 term is much lower (around 15 ns/pair for lshCount=1024),
    // at a cost of additional O(N) work (typically 40 ms per cell for lshCount=1024).
    // As a result, this can be much faster for large numbers of cells.
    // The error of the approximation is controlled by lshCount.
    // The maximum standard deviation of the computed similarity is (pi/2)/sqrt(lshCount),
    // or about 0.05 for lshCount=1024.
    // The standard deviation decreases as the similarity increases. It becomes
    // zero when the similarity is 1. For similarity 0.5, the standard deviation is 82%
    // of the standard deviation at similarity 0.
    void findSimilarPairs1(
    	const string& cellSetName,	// The name of the cell set to be used.
        const string& name,         // The name of the SimilarPairs object to be created.
        size_t k,                   // The maximum number of similar pairs to be stored for each cell.
        double similarityThreshold, // The minimum similarity for a pair to be stored.
		size_t lshCount,		    // The number of LSH vectors to use.
		unsigned int seed			// The seed used to generate the LSH vectors.
		);


    // Find similar cell pairs using LSH, without looping over all pairs.
    // See the beginning of ExpressionMatrixLsh.cpp for more information.
    // This implementation requires lshRowCount to be a power of 2 not greater than 64.
    void findSimilarPairs2(
   		const string& cellSetName,	// The name of the cell set to be used.
        const string& name,         // The name of the SimilarPairs object to be created.
        size_t k,                   // The maximum number of similar pairs to be stored for each cell.
        double similarityThreshold, // The minimum similarity for a pair to be stored.
		size_t lshBandCount,		// The number of LSH bands, each generated using lshRowCount LSH vectors.
		size_t lshRowCount,         // The number of LSH vectors in each of the lshBandCount LSH bands.
		unsigned int seed,			// The seed used to generate the LSH vectors.
		double loadFactor           // Of the hash table used to assign cells to bucket.
		);


    // Dump cell to csv file a set of similar cell pairs.
    void writeSimilarPairs(const string& name) const;

    // Create a new graph.
    // Graphs are not persistent (they are stored in memory only).
    void createCellSimilarityGraph(
        const string& graphName,            // The name of the graph to be created. This is used as a key in the graph map.
        const string& cellSetName,          // The cell set to be used.
        const string& similarPairsName,     // The name of the SimilarPairs object to be used to create the graph.
        double similarityThreshold,         // The minimum similarity to create an edge.
        size_t k                           // The maximum number of neighbors (k of the k-NN graph).
     );


private:

    // The directory that contains the binary data for this Expression matrix.
    string directoryName;

    // A StringTable containing the gene names.
    // Given a GeneId (an integer), it can find the gene name.
    // Given the gene name, it can find the corresponding GeneId.
    MemoryMapped::StringTable<GeneId> geneNames;

    // Vector containing fixed size information for each cell.
    // Variable size information (meta data and expression counts)
    // are stored separately - see below.
    MemoryMapped::Vector<Cell> cells;

    // A StringTable containing the cell names.
    // Given a CellId (an integer), it can find the cell name.
    // Given the cell name, it can find the corresponding CellId.
    // The name of reach cell is also stored as the first entry
    // in the meta data for the cell, called "cellName".
    MemoryMapped::StringTable<CellId> cellNames;

    // The meta data for each cell.
    // For each cell we store pairs of string ids for each meta data (name, value) pair.
    // The corresponding strings are stored in cellMetaDataNames and cellMetaDataValues.
    // The first (name, value) pair for each cell contains name = "CellName"
    // and value = the name of the cell.
    MemoryMapped::VectorOfLists< pair<StringId, StringId> > cellMetaData;
    MemoryMapped::StringTable<StringId> cellMetaDataNames;
    MemoryMapped::StringTable<StringId> cellMetaDataValues;

    // The number of cells that use each of the cell meta data names.
    // This is maintained to always have the same size as cellMetaDataNames,
    // and it is indexed by the StringId.
    MemoryMapped::Vector<CellId> cellMetaDataNamesUsageCount;
    void incrementCellMetaDataNameUsageCount(StringId);
    void decrementCellMetaDataNameUsageCount(StringId);

    // The expression counts for each cell. Stored in sparse format,
    // each with the GeneId it corresponds to.
    // For each cell, they are stored sorted by increasing GeneId.
    // This is indexed by the CellId.
    MemoryMapped::VectorOfVectors<pair<GeneId, float>, uint64_t> cellExpressionCounts;

    // Return the raw cell count for a given cell and gene.
    // This does a binary search in the cellExpressionCounts for the given cell.
    float getExpressionCount(CellId, GeneId) const;

    // We also separately store the largest expression counts for each cell.
    // This is organized in the same way as cellExpressionCounts above.
    // This is used for fast, approximate computations of cell similarities.
    // The threshold for storing an expression count is different for each cell.
    // For each cell, we store in the Cell object the number of expression
    // counts neglected and the value of the largest expression count neglected.
    // With these we compute error bounds for approximate similarity
    // computations.
    MemoryMapped::VectorOfVectors<pair<GeneId, float>, uint64_t> largeCellExpressionCounts;

    // Functions used to implement HttpServer functionality.
public:
    void explore(const ServerParameters& serverParameters);
private:
    ServerParameters serverParameters;
    void processRequest(const vector<string>& request, ostream& html);
    typedef void (ExpressionMatrix::*ServerFunction)(const vector<string>& request, ostream& html);
    map<string, ServerFunction> serverFunctionTable;
    void fillServerFunctionTable();
    void writeNavigation(ostream& html);
    void writeNavigation(ostream& html, const string& text, const string& url);
    void exploreSummary(const vector<string>& request, ostream& html);
    void exploreHashTableSummary(const vector<string>& request, ostream&);
    void exploreGene(const vector<string>& request, ostream& html);
    void exploreGeneInformationContent(const vector<string>& request, ostream& html);
    void exploreCell(const vector<string>& request, ostream& html);
    ostream& writeCellLink(ostream&, CellId, bool writeId=false);
    ostream& writeCellLink(ostream&, const string& cellName, bool writeId=false);
    ostream& writeGeneLink(ostream&, GeneId, bool writeId=false);
    ostream& writeGeneLink(ostream&, const string& geneName, bool writeId=false);
    ostream& writeMetaDataSelection(ostream&, const string& selectName, bool multiple) const;
    ostream& writeMetaDataSelection(ostream&, const string& selectName, const set<string>& selected, bool multiple) const;
    ostream& writeMetaDataSelection(ostream&, const string& selectName, const vector<string>& selected, bool multiple) const;
    void compareTwoCells(const vector<string>& request, ostream& html);
    void exploreCellSets(const vector<string>& request, ostream& html);
    void exploreCellSet(const vector<string>& request, ostream& html);
    void createCellSetUsingMetaData(const vector<string>& request, ostream& html);
    void createCellSetIntersectionOrUnion(const vector<string>& request, ostream& html);
    void downsampleCellSet(const vector<string>& request, ostream& html);
    ostream& writeCellSetSelection(ostream& html, const string& selectName, bool multiple) const;
    ostream& writeCellSetSelection(ostream& html, const string& selectName, const set<string>& selected, bool multiple) const;
    ostream& writeGeneSetSelection(ostream& html, const string& selectName, bool multiple) const;
    ostream& writeGeneSetSelection(ostream& html, const string& selectName, const set<string>& selected, bool multiple) const;
    ostream& writeGraphSelection(ostream& html, const string& selectName, bool multiple) const;
    ostream& writeNormalizationSelection(ostream& html, const string& selectedNormalizationOption) const;
	void removeCellSet(const vector<string>& request, ostream& html);
    void exploreGraphs(const vector<string>& request, ostream& html);
    void compareGraphs(const vector<string>& request, ostream& html);
    void exploreGraph(const vector<string>& request, ostream& html);
    void clusterDialog(const vector<string>& request, ostream& html);
    void cluster(const vector<string>& request, ostream& html);
    void createNewGraph(const vector<string>& request, ostream& html);
    void removeGraph(const vector<string>& request, ostream& html);
    void getAvailableSimilarPairs(vector<string>&) const;
    void exploreMetaData(const vector<string>& request, ostream& html);
    void metaDataHistogram(const vector<string>& request, ostream& html);
    void metaDataContingencyTable(const vector<string>& request, ostream& html);
    void removeMetaData(const vector<string>& request, ostream& html);



    // Class used by exploreGene.
    class ExploreGeneData {
    public:
        CellId cellId;
        float rawCount;
        float count1;   // L1 normalized.
        float count2;   // L2 normalized.
        bool operator<(const ExploreGeneData& that) const
        {
            return count2 > that.count2;    // Greater counts comes first
        }
    };

    // Return a cell id given a string.
    // The string can be a cell name or a CellId (an integer).
    // Returns invalidCellId if the cell was not found.
    CellId cellIdFromString(const string& s);

    // Return a gene id given a string.
    // The string can be a gene name or GeneId (a string).
    // Returns invalidGeneId if the gene was not found.
    GeneId geneIdFromString(const string& s);

    // Functionality to define and maintain cell sets.
    CellSets cellSets;

    // Gene sets, keyed by gene set name.
    // This always contains gene set AllGenes.
    map<string, GeneSet> geneSets;



    // Data and functions used for Locality Sensitive Hashing (LSH).
    // LSH is used for efficiently finding pairs of similar cells.
    // See Chapter 3 of Leskovec, Rajaraman, Ullman, Mining of Massive Datasets,
    // Cambridge University Press, 2014, also freely downloadable here:
    // http://www.mmds.org/#ver21
    // and in particular sections 3.4 through 3.7.

    // Generate random unit vectors in gene space.
    // These are vectors of dimension equal to the number of genes,
    // and with unit L2-norm (the sum of the square if the components is 1).
    // These vectors are organized by band and row (see section 3.4.1 of the
    // book referenced above). There are lshBandCount bands and lshRowCount
    // rows per band, for a total lshBandCount*lshRowCount random vectors.
    // Each of these vectors defines an hyperplane orthogonal to it.
    // As described in section 3.7.2 of the book referenced above,
    // each hyperplane provides a function of a locality-sensitive function.
    void generateLshVectors(
    	size_t lshBandCount,
		size_t lshRowCount,
		unsigned int seed,
		vector< vector< vector<double> > >& lshVectors	// Indexed by [band][row][geneId]
        ) const;


#if 0
    // Orthogonalize the LSH vectors in groups of k.
    // Ji et al. (2012) have shown that orthogonalization of
    // groups of LSH vectors can result in significant reduction of the
    // variance of the distance estimate provided by LSH.
    // See J. Ji, J. Li, S. Yan, B. Zhang, and Q. Tian,
    // Super-Bit Locality-Sensitive Hashing, In NIPS, pages 108–116, 2012.
    // https://pdfs.semanticscholar.org/64d8/3ccbcb1d87bfafee57f0c2d49043ee3f565b.pdf
    // This did not seem to give any benefit, so I turned it off to eliminate the
    // dependency on Lapack.
    void orthogonalizeLshVectors(
		vector< vector< vector<double> > >& lshVectors,
		size_t k
        ) const;
#endif


	// Compute the scalar product of an LSH vector with the normalized expression counts of a cell.
    double computeExpressionCountScalarProduct(CellId, const vector<double>& v) const;

    // Approximate computation of the angle between the expression vectors of two cells
    // using Locality Sensitive Hashing (LSH).
    // The approximate similarity can be computed as the cosine of this angle.
    // This recomputes every time the scalar product of the normalized cell expression vector
    // with the LSH vectors.
    double computeApproximateLshCellAngle(
    	const vector< vector< vector<double> > >& lshVectors,
		CellId,
		CellId) const;

    // Approximate computation of the angle between the expression vectors of two cells
    // using Locality Sensitive Hashing (LSH).
    double computeApproximateLshCellAngle(
		const BitSet& signature0,
		const BitSet& signature1,
		double bitCountInverse) const;

    // Given LSH vectors, compute the LSH signature of all cells.
	// The LSH signature of a cell is a bit vector with one bit for each of the LSH vectors.
	// Each bit is 1 if the scalar product of the cell expression vector
    // (normalized to zero mean and unit variance) with the
    // the LSH vector is positive, and 0 otherwise.
    void computeCellLshSignatures(
    	const vector< vector< vector<double> > >& lshVectors,
		vector<BitSet>& signatures
		) const;

    // Same as above, but only for a set of cells given in a vector of cell ids (cell set).
    void computeCellLshSignatures(
    	const vector< vector< vector<double> > >& lshVectors,
    	const MemoryMapped::Vector<CellId>& cellSet,
		vector<BitSet>& signatures
		) const;

	// Write to a csv file statistics of the LSH signatures..
	void writeLshSignatureStatistics(size_t bitCount, const vector<BitSet>& signatures) const;

public:
    // Approximate computation of the similarity between two cells using
    // Locality Sensitive Hashing (LSH).
    // Not to be used for code where performance is important,
    // because it recomputes the LSH vector every time.
    double computeApproximateLshCellSimilarity(
		size_t lshBandCount,
		size_t lshRowCount,
		unsigned int seed,
		CellId,
		CellId) const;

    // Write a csv file containing, for every pair of cells,
    // the exact similarity and the similarity computed using LSH.
    void writeLshSimilarityComparisonSlow(
		size_t lshBandCount,
		size_t lshRowCount,
		unsigned int seed
		) const;
    void writeLshSimilarityComparison(
		size_t lshBandCount,
		size_t lshRowCount,
		unsigned int seed
		) const;




public:

    // Create a new cell set that contains cells for which
    // the value of a specified meta data field matches
    // a given regular expression.
    // Return true if successful, false if a cell set with
    // the specified name already exists.
    bool createCellSetUsingMetaData(
        const string& cellSetName,          // The name of the cell set to be created.
        const string& metaDataFieldName,    // The name of the meta data field to be used.
        const string& regex                 // The regular expression that must be matched for a cell to be added to the set.
        );

    // Create a new cell set as the intersection or union of two or more existing cell sets.
    // The input cell sets are specified comma separated in the first argument.
    // Return true if successful, false if one of the input cell sets does not exist
    // or the output cell set already exists.
    bool createCellSetIntersection(const string& inputSets, const string& outputSet);
    bool createCellSetUnion(const string& inputSets, const string& outputSet);
    bool createCellSetIntersectionOrUnion(const string& inputSets, const string& outputSet, bool doUnion);

    // Create a new cell set by downsampling an existing cell set
    // Return true if successful, false if the input cell set does not exist.
    bool downsampleCellSet(
    	const string& inputCellSetName,
		const string& newCellSetName,
		double probability,
		int seed);

    // The cell similarity graphs.
    // This is not persistent (lives in memory only).
    map<string, pair<GraphInformation, boost::shared_ptr<CellSimilarityGraph> > > graphs;

    // Store the cluster ids in a graph in a meta data field.
    void storeClusterId(const string& metaDataName, const CellSimilarityGraph&);


};



#endif
