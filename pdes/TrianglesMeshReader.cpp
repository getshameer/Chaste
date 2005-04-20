#include "TrianglesMeshReader.hpp"

TrianglesMeshReader::TrianglesMeshReader(std::string pathBaseName)
{
	
	mPathBaseName=pathBaseName;

	std::string nodeFileName=pathBaseName+".node";
	std::ifstream nodeFile(nodeFileName.c_str());
	
	
	if (!nodeFile.is_open())
	{
		throw Exception("Could not open node file "+nodeFileName+" .");
	}
	
	int numAttributes, maxBoundaryMarkers;
	nodeFile >> mNumNodes >> mDimension >> numAttributes >> maxBoundaryMarkers;
	
	mppNodes = MakeDoubleDynamicArray(mNumNodes,3);
	
	nodeFile.close();
}

TrianglesMeshReader::~TrianglesMeshReader()
{
}
