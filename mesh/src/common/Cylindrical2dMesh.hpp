#ifndef _CYLINDRICAL2DMESH_HPP_
#define _CYLINDRICAL2DMESH_HPP_

//#include <boost/serialization/access.hpp>
//#include <boost/serialization/base_object.hpp>

#include "ConformingTetrahedralMesh.cpp"
#include "NodeMap.hpp"

//#include <boost/serialization/export.hpp>// at end of includes

class Cylindrical2dMesh : public ConformingTetrahedralMesh<2, 2>
{
    friend class TestCylindrical2dMesh;
private:
    
    /** The circumference of the cylinder */
    double mWidth;
    /** The top of the cylinder (y-coord) */
    double mTop;
    /** The bottom of the cylinder (y-coord) */
    double mBottom;
    
    /** Used during ReMesh **/
    // The left nodes which have been mirrored
    std::vector<unsigned> mLeftOriginals;
    // The image nodes relating to these left nodes (on right of mesh)
    std::vector<unsigned> mLeftImages;
    // The right nodes which have been mirrored
    std::vector<unsigned> mRightOriginals;
    // The image nodes relating to these right nodes (on left of mesh)
    std::vector<unsigned> mRightImages;
    
    /** The indices of nodes on the top boundary */
    std::vector<unsigned > mTopHaloNodes;
    /** The indices of nodes on the bottom boundary */
    std::vector<unsigned > mBottomHaloNodes;
    
    void ReplaceImageWithRealNodeOnElement(Element<2,2>* pElement, std::vector<unsigned> &rImageNodes, std::vector<unsigned> &rOriginalNodes, unsigned nodeIndex ) ;
    
//    friend class boost::serialization::access;
//    template<class Archive>
//    void serialize(Archive & archive, const unsigned int version)
//    {
//        archive & boost::serialization::base_object<ConformingTetrahedralMesh>(*this);
//        archive & mWidth;
//        archive & mTop;
//        archive & mBottom;
//    }
    
public:
    
   Cylindrical2dMesh(double width);
         
    ~Cylindrical2dMesh()
    {
    }
    
    void UpdateTopAndBottom();
    void CreateHaloNodes();
    void CreateMirrorNodes();
    void ReMesh(NodeMap &map);
    void ReconstructCylindricalMesh();
    void DeleteHaloNodes();
    c_vector<double, 2> GetVectorFromAtoB(const c_vector<double, 2>& rLocation1, const c_vector<double, 2>& rLocation2);
    void SetNode(unsigned index, Point<2> point, bool concreteMove);
    bool IsThisIndexInList(const unsigned& rNodeIndex, const std::vector<unsigned>& rListOfNodes);
    double GetWidth(const unsigned& rDimension);
    unsigned AddNode(Node<2> *pNewNode);
    
};

//BOOST_CLASS_EXPORT(Cylindrical2dMesh);

#endif //_CYLINDRICAL2DMESH_HPP_
