#ifndef _TESTELEMENT_HPP_
#define _TESTELEMENT_HPP_

#include "Point.hpp"
#include "Node.hpp"
#include "Element.hpp"
#include <cxxtest/TestSuite.h>
//#include <iostream>

#include <vector>

class TestElement : public CxxTest::TestSuite 
{
	/**
	 * Create a node with given index that has a point at the origin and is
	 * not on the boundary.
	 * 
	 * @param index The global index of the created node.
	 * @returns A pointer to a new 3d node.
	 */
	Node<3> *CreateZeroPointNode(int index)
	{
		Point<3> point(0, 0, 0);
		Node<3> *pnode = new Node<3>(index, point, false);
		return pnode;
	}

public:
	void TestConstructionForQuadraticBasisFunctions()
	{
		std::vector<Node<3>*> nodes;
		nodes.push_back(new Node<3>(0, false, 0.0, 0.0, 0.0));
		nodes.push_back(new Node<3>(1, false, 1.0, 0.0, 0.0));
		nodes.push_back(new Node<3>(2, false, 0.0, 1.0, 0.0));
		nodes.push_back(new Node<3>(3, false, 0.0, 0.0, 1.0));
		nodes.push_back(new Node<3>(4, false, 0.5, 0.0, 0.0));
		nodes.push_back(new Node<3>(5, false, 0.5, 0.5, 0.0));
		nodes.push_back(new Node<3>(6, false, 0.0, 0.5, 0.0));
		nodes.push_back(new Node<3>(7, false, 0.0, 0.0, 0.5));
		nodes.push_back(new Node<3>(8, false, 0.5, 0.0, 0.5));
		nodes.push_back(new Node<3>(9, false, 0.0, 0.5, 0.5));
		Element<3,3> element(nodes, 2,true);
        
		// Check nodes on the new element have the right indices
		for (int i=0; i<10; i++)
		{
			TS_ASSERT_EQUALS(element.GetNodeGlobalIndex(i), i);
		}
		// Check lower order elements are created with the expected nodes.
		// Not sure if we really want to specify this, but it ensures nothing
		// has changed from the earlier code, just in case.
		for (int i=0; i < 4; i++)
        {
            for(int j=0; j < 3; j++)
            {
               TS_ASSERT_EQUALS(element.GetLowerOrderElement(i)->GetNodeGlobalIndex(j),(i+j+1) % 4);
               if((i==0 && j==0) || (i==2 && j==2)|| (i==3 && j==1))
               {
               		TS_ASSERT_DELTA(element.GetLowerOrderElement(i)->GetNodeLocation(j, 0), 1.0, 1e-12);
               }
               else
               {
               		TS_ASSERT_DELTA(element.GetLowerOrderElement(i)->GetNodeLocation(j, 0), 0.0, 1e-12);
               }
            }
        }
        TS_ASSERT_EQUALS(element.GetLowerOrderElement(0)->GetLowerOrderElement(0)->GetNodeGlobalIndex(0),2);
        TS_ASSERT_EQUALS(element.GetLowerOrderElement(0)->GetLowerOrderElement(0)->GetNodeGlobalIndex(1),3);
        TS_ASSERT_EQUALS(element.GetLowerOrderElement(0)->GetLowerOrderElement(1)->GetNodeGlobalIndex(1),1);
        TS_ASSERT_EQUALS(element.GetLowerOrderElement(0)->GetLowerOrderElement(0)->GetLowerOrderElement(0)->GetNodeGlobalIndex(0),3);
	}
	
	void TestConstructionForLinearBasisFunctions()
	{
		std::vector<Node<3>*> cornerNodes;
		cornerNodes.push_back(new Node<3>(0, false, 0.0, 0.0, 0.0));
		cornerNodes.push_back(new Node<3>(1, false, 1.0, 0.0, 0.0));
		cornerNodes.push_back(new Node<3>(2, false, 0.0, 1.0, 0.0));
		cornerNodes.push_back(new Node<3>(3, false, 0.0, 0.0, 1.0));
		Element<3,3> element(cornerNodes, 1, true);
				
		// Check nodes on the new element have the right indices
		for (int i=0; i<4; i++)
		{
			TS_ASSERT_EQUALS(element.GetNodeGlobalIndex(i), i);
		}
        
		// Check lower order elements are created with the expected nodes.
		// Not sure if we really want to specify this, but it ensures nothing
		// has changed from the earlier code, just in case.
		for (int i=0; i < 4; i++)
        {
            for (int j=0; j < 3; j++)
            {
               TS_ASSERT_EQUALS(element.GetLowerOrderElement(i)->GetNodeGlobalIndex(j),(i+j+1) % 4);
               if ((i==0 && j==0) || (i==2 && j==2) || (i==3 && j==1))
               {
               		TS_ASSERT_DELTA(element.GetLowerOrderElement(i)->GetNodeLocation(j, 0), 1.0, 1e-12);
               }
               else
               {
               		TS_ASSERT_DELTA(element.GetLowerOrderElement(i)->GetNodeLocation(j, 0), 0.0, 1e-12);
               }
            }
        }
        TS_ASSERT_EQUALS(element.GetLowerOrderElement(0)->GetLowerOrderElement(0)->GetNodeGlobalIndex(0),2);
        TS_ASSERT_EQUALS(element.GetLowerOrderElement(0)->GetLowerOrderElement(0)->GetNodeGlobalIndex(1),3);
        TS_ASSERT_EQUALS(element.GetLowerOrderElement(0)->GetLowerOrderElement(1)->GetNodeGlobalIndex(1),1);
        TS_ASSERT_EQUALS(element.GetLowerOrderElement(0)->GetLowerOrderElement(0)->GetLowerOrderElement(0)->GetNodeGlobalIndex(0),3);
    }

	void TestJacobian()
	{
		// 1d
		std::vector<Node<1>*> nodes1d;
		nodes1d.push_back(new Node<1>(0, false, 2.0));
		nodes1d.push_back(new Node<1>(1, false, 2.5));
		Element<1,1> element1d(nodes1d);
		const c_matrix<double, 1, 1> *J1d = element1d.GetJacobian();
		TS_ASSERT_DELTA((*J1d)(0,0), 0.5, 1e-12);
		
		double Det1d = element1d.GetJacobianDeterminant();
		TS_ASSERT_DELTA(Det1d, 0.5, 1e-12);
		const c_matrix<double, 1, 1> *J1dinv = element1d.GetInverseJacobian();
		TS_ASSERT_DELTA((*J1dinv)(0,0), 2.0, 1e-12);
				
		delete nodes1d[0];
		delete nodes1d[1];
		// 2d easy
		
		std::vector<Node<2>*> nodes2d;
		nodes2d.push_back(new Node<2>(0, false, 0.0, 0.0));
		nodes2d.push_back(new Node<2>(1, false, 1.0, 0.0));
		nodes2d.push_back(new Node<2>(2, false, 0.0, 1.0));
		Element<2,2> element2d(nodes2d);
		const c_matrix<double, 2, 2> *J2d = element2d.GetJacobian();
		TS_ASSERT_DELTA((*J2d)(0,0), 1.0, 1e-12);
		TS_ASSERT_DELTA((*J2d)(0,1), 0.0, 1e-12);
		TS_ASSERT_DELTA((*J2d)(1,0), 0.0, 1e-12);
		TS_ASSERT_DELTA((*J2d)(1,1), 1.0, 1e-12);
		
		delete nodes2d[0];
		delete nodes2d[1];
		delete nodes2d[2];

		//2d general
		std::vector<Node<2>*> nodes2d2;
		nodes2d2.push_back(new Node<2>(0, false, 1.0, -2.0));
		nodes2d2.push_back(new Node<2>(1, false, 4.0, -3.0));
		nodes2d2.push_back(new Node<2>(2, false, 2.0, -1.0));
		Element<2,2> element2d2(nodes2d2);
		const c_matrix<double, 2, 2> *J2d2 = element2d2.GetJacobian();
		TS_ASSERT_DELTA((*J2d2)(0,0), 3.0, 1e-12);
		TS_ASSERT_DELTA((*J2d2)(0,1), 1.0, 1e-12);
		TS_ASSERT_DELTA((*J2d2)(1,0), -1.0, 1e-12);
		TS_ASSERT_DELTA((*J2d2)(1,1), 1.0, 1e-12);
		
		double Det2d = element2d2.GetJacobianDeterminant();
		TS_ASSERT_DELTA(Det2d, 4.0, 1e-12);
		const c_matrix<double, 2, 2> *J2d2inv = element2d2.GetInverseJacobian();
		TS_ASSERT_DELTA((*J2d2inv)(0,0), 0.25, 1e-12);
		TS_ASSERT_DELTA((*J2d2inv)(0,1), -0.25, 1e-12);
		TS_ASSERT_DELTA((*J2d2inv)(1,0), 0.25, 1e-12);
		TS_ASSERT_DELTA((*J2d2inv)(1,1), 0.75, 1e-12);

		delete nodes2d2[0];
		delete nodes2d2[1];
		delete nodes2d2[2];

		
		// 3d easy
		std::vector<Node<3>*> nodes3d;
		nodes3d.push_back(new Node<3>(0, false, 0.0, 0.0, 0.0));
		nodes3d.push_back(new Node<3>(1, false, 1.0, 0.0, 0.0));
		nodes3d.push_back(new Node<3>(2, false, 0.0, 1.0, 0.0));
		nodes3d.push_back(new Node<3>(3, false, 0.0, 0.0, 1.0));
		Element<3,3> element3d(nodes3d, true);
		const c_matrix<double, 3, 3> *J3d = element3d.GetJacobian();
		TS_ASSERT_DELTA((*J3d)(0,0), 1.0, 1e-12);
		TS_ASSERT_DELTA((*J3d)(0,1), 0.0, 1e-12);
		TS_ASSERT_DELTA((*J3d)(0,2), 0.0, 1e-12);
		TS_ASSERT_DELTA((*J3d)(1,0), 0.0, 1e-12);
		TS_ASSERT_DELTA((*J3d)(1,1), 1.0, 1e-12);
		TS_ASSERT_DELTA((*J3d)(1,2), 0.0, 1e-12);
		TS_ASSERT_DELTA((*J3d)(2,0), 0.0, 1e-12);
		TS_ASSERT_DELTA((*J3d)(2,1), 0.0, 1e-12);
		TS_ASSERT_DELTA((*J3d)(2,2), 1.0, 1e-12);
		
		delete nodes3d[0];
		delete nodes3d[1];
		delete nodes3d[2];
    	delete nodes3d[3];
		
		
		// 3d general
		std::vector<Node<3>*> nodes3d2;
		nodes3d2.push_back(new Node<3>(0, false, 1.0, 2.0, 3.0));
		nodes3d2.push_back(new Node<3>(1, false, 2.0, 1.0, 3.0));
		nodes3d2.push_back(new Node<3>(2, false, 5.0, 5.0, 5.0));
		nodes3d2.push_back(new Node<3>(3, false, 0.0, 3.0, 4.0));
		Element<3,3> element3d2(nodes3d2, true);
		const c_matrix<double, 3, 3> *J3d2 = element3d2.GetJacobian();
		TS_ASSERT_DELTA((*J3d2)(0,0), 1.0, 1e-4);
		TS_ASSERT_DELTA((*J3d2)(0,1), 4.0, 1e-4);
		TS_ASSERT_DELTA((*J3d2)(0,2), -1.0, 1e-4);
		TS_ASSERT_DELTA((*J3d2)(1,0), -1.0, 1e-4);
		TS_ASSERT_DELTA((*J3d2)(1,1), 3.0, 1e-4);
		TS_ASSERT_DELTA((*J3d2)(1,2), 1.0, 1e-4);
		TS_ASSERT_DELTA((*J3d2)(2,0), 0.0, 1e-4);
		TS_ASSERT_DELTA((*J3d2)(2,1), 2.0, 1e-4);
		TS_ASSERT_DELTA((*J3d2)(2,2), 1.0, 1e-4);
		
		double Det3d2 = element3d2.GetJacobianDeterminant();
		TS_ASSERT_DELTA(Det3d2, 7.0, 1e-4);
		const c_matrix<double, 3, 3> *J3d2inv = element3d2.GetInverseJacobian();
		TS_ASSERT_DELTA((*J3d2inv)(0,0), 1.0/7.0, 1e-4);
		TS_ASSERT_DELTA((*J3d2inv)(0,1), -6.0/7.0, 1e-4);
		TS_ASSERT_DELTA((*J3d2inv)(0,2), 1.0, 1e-4);
		TS_ASSERT_DELTA((*J3d2inv)(1,0), 1.0/7.0, 1e-4);
		TS_ASSERT_DELTA((*J3d2inv)(1,1), 1.0/7.0, 1e-4);
		TS_ASSERT_DELTA((*J3d2inv)(1,2), 0.0, 1e-4);
		TS_ASSERT_DELTA((*J3d2inv)(2,0), -2.0/7.0, 1e-4);
		TS_ASSERT_DELTA((*J3d2inv)(2,1), -2.0/7.0, 1e-4);
		TS_ASSERT_DELTA((*J3d2inv)(2,2), 1.0, 1e-4);
		
		delete nodes3d2[0];
		delete nodes3d2[1];
		delete nodes3d2[2];
    	delete nodes3d2[3];
	}
	
    void TestNodeToElementConversion(void)
    {
        Point<1> point1(1.0);
        Point<2> point2(2.0,-1.0);
        
        Node<1> node1(0, point1);
        Node<2> node2(0, point2);
        
        Element<0,1> element1(&node1);
        Element<0,2> element2(&node2);
        
        TS_ASSERT_EQUALS(element1.GetNode(0)->GetPoint()[0], point1[0]);
        
        TS_ASSERT_EQUALS(element2.GetNode(0)->GetPoint()[0], point2[0]);
        TS_ASSERT_EQUALS(element2.GetNode(0)->GetPoint()[1], point2[1]);
    }
    
    void TestElementSwapsNodesIfJacobianIsNegative()
    {
        Point<1> a0(0),    a1(1);
        Point<2> b0(0,0),  b1(1,0)  ,b2(0,1);
        Point<3> c0(0,0,0),c1(1,0,0),c2(0,1,0),c3(0,0,1);
        
        Node<1>  na0(0,a0), na1(1,a1);
        Node<2>  nb0(0,b0), nb1(1,b1), nb2(2,b2);
        Node<3>  nc0(0,c0), nc1(1,c1), nc2(2,c2), nc3(3,c3);


        ////////////////////////////////////////////
        // 1d
        ////////////////////////////////////////////        
        std::vector<Node<1>*> nodes_1d_correct;
        nodes_1d_correct.push_back(&na0);
        nodes_1d_correct.push_back(&na1);

        std::vector<Node<1>*> nodes_1d_incorrect;
        nodes_1d_incorrect.push_back(&na1);
        nodes_1d_incorrect.push_back(&na0);

        Element<1,1>   e_1d_correct_orientation(nodes_1d_correct);
        Element<1,1> e_1d_incorrect_orientation(nodes_1d_incorrect); 

        // index of second node should be 1
        TS_ASSERT_EQUALS( e_1d_correct_orientation.GetNode(1)->GetIndex(), 1);
        // index of second node for incorrect orientation element should also be 1 
        // because the element should have swapped the nodes around
        TS_ASSERT_EQUALS( e_1d_incorrect_orientation.GetNode(1)->GetIndex(), 1);


        ////////////////////////////////////////////
        // 2d
        ////////////////////////////////////////////        
        std::vector<Node<2>*> nodes_2d_correct;
        nodes_2d_correct.push_back(&nb0);
        nodes_2d_correct.push_back(&nb1);
        nodes_2d_correct.push_back(&nb2);

        std::vector<Node<2>*> nodes_2d_incorrect;
        nodes_2d_incorrect.push_back(&nb1);
        nodes_2d_incorrect.push_back(&nb0);
        nodes_2d_incorrect.push_back(&nb2);

        Element<2,2>   e_2d_correct_orientation(nodes_2d_correct);
        Element<2,2> e_2d_incorrect_orientation(nodes_2d_incorrect); 

        // index of last node should be 2
        TS_ASSERT_EQUALS( e_2d_correct_orientation.GetNode(2)->GetIndex(), 2);

        // index of last node for incorrect orientation element should be 0 
        // because the element should have swapped the last two nodes around
        TS_ASSERT_EQUALS( e_2d_incorrect_orientation.GetNode(2)->GetIndex(), 0);


        ////////////////////////////////////////////
        // 3d
        ////////////////////////////////////////////        
        std::vector<Node<3>*> nodes_3d_correct;
        nodes_3d_correct.push_back(&nc0);
        nodes_3d_correct.push_back(&nc1);
        nodes_3d_correct.push_back(&nc2);
        nodes_3d_correct.push_back(&nc3);

        std::vector<Node<3>*> nodes_3d_incorrect;
        nodes_3d_incorrect.push_back(&nc0);
        nodes_3d_incorrect.push_back(&nc1);
        nodes_3d_incorrect.push_back(&nc3);
        nodes_3d_incorrect.push_back(&nc2);

        Element<3,3>   e_3d_correct_orientation(nodes_3d_correct);
        Element<3,3> e_3d_incorrect_orientation(nodes_3d_incorrect); 

        // index of last node should be 3
        TS_ASSERT_EQUALS( e_3d_correct_orientation.GetNode(3)->GetIndex(), 3);

        // index of last node for incorrect orientation element should be 3 
        // because the element should have swapped the last two nodes around
        TS_ASSERT_EQUALS( e_3d_incorrect_orientation.GetNode(3)->GetIndex(), 3);

    }
};

#endif //_TESTELEMENT_HPP_
