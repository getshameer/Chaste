#ifndef _NODE_HPP_
#define _NODE_HPP_

#include "Point.hpp"

template<int SPACE_DIM>
class Node
{
private:
    int mIndex;
    Point<SPACE_DIM> mPoint;
    
    bool mIsBoundaryNode;
    
public:

	Node(int index, Point<SPACE_DIM> point, bool isBoundaryNode=false)
	{
		mPoint = point;
		mIndex = index;
		mIsBoundaryNode = isBoundaryNode;
	}

	void SetPoint(Point<SPACE_DIM> point)
	{
		mPoint = point;
	}
	
	void SetIndex(int index)
	{
		mIndex = index;
	}
		
	Point<SPACE_DIM> GetPoint()
	{
		return mPoint;
	}
	
	int GetIndex()
	{
		return mIndex;
	}
	
	void SetAsBoundaryNode(bool value=true)
	{
		mIsBoundaryNode = value;
	}
	
	bool IsBoundaryNode()
	{
		return mIsBoundaryNode;
	}
};


#endif //_NODE_HPP_
