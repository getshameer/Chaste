#include "SingletonWntGradient.hpp"
#include "Exception.hpp"
#include <iostream>
#include <cassert>
#include "CancerParameters.hpp"


/** Pointer to the single instance */
SingletonWntGradient* SingletonWntGradient::mpInstance = NULL;

/**
 * Return a pointer to the simulation time object.
 * The first time this is called the simulation time object is created.
 * */
SingletonWntGradient* SingletonWntGradient::Instance()
{
    if (mpInstance == NULL)
    {
        mpInstance = new SingletonWntGradient;
    }
    return mpInstance;
}


/**
 * @param type the types of SingletonWntGradient defined in WntGradientTypes.hpp
 */
SingletonWntGradient::SingletonWntGradient()
 :  mpCancerParams(CancerParameters::Instance()),
    mpCrypt(NULL),
    mGradientType(NONE),
    mTypeSet(false)
{
    // Make sure there's only one instance - enforces correct serialization
    assert(mpInstance == NULL);
    
    mUseConstantWntValueForTesting = false;
    mConstantWntValueForTesting = 0.0;
}

SingletonWntGradient::~SingletonWntGradient()
{}

void SingletonWntGradient::Destroy()
{
    if (mpInstance)
    {
        delete mpInstance;
        mpInstance = NULL;
    }
}


double SingletonWntGradient::GetWntLevel(MeinekeCryptCell* pCell)
{
    if(mUseConstantWntValueForTesting)  // to test a cell and cell cycle models without a crypt
    {
        return mConstantWntValueForTesting;
    }

    assert(mpCrypt!=NULL);
    assert(mTypeSet);
    assert(pCell!=NULL);
    double height=(mpCrypt->GetLocationOfCell(*pCell))(1);// y-coord.
    return GetWntLevel(height);
}

void SingletonWntGradient::SetCrypt(Crypt<2>& rCrypt)
{
    mpCrypt=&rCrypt;
    rCrypt.InitialiseCells();
}

void SingletonWntGradient::SetType(WntGradientType type)
{
    if(mTypeSet==true)
    {
        EXCEPTION("Destroy has not been called");
    }
    mGradientType = type;
    mTypeSet = true;
}


/**
 * @param height The height of the cell we want the Wnt concentration of
 * @return wnt_level The concentration of Wnt for this cell (dimensionless)
 */
double SingletonWntGradient::GetWntLevel(double height)
{
    double wnt_level = -1.0;
    
    if (mGradientType==NONE)
    {
        wnt_level=0.0;
    }
    
    // The first Wnt gradient to try
    if (mGradientType==LINEAR)
    {
        double crypt_height = mpCancerParams->GetCryptLength();
        //std::cout << "Crypt Height = " << crypt_height << " height = " << height << "\n";
        
        if ((height >= -1e-9) && (height < crypt_height))
        {
            wnt_level = 1.0 - height/crypt_height;
        }
        else
        {
            wnt_level = 0.0;
        }
    }
    
    // An offset Wnt gradient - reaches zero at 2/3 of way up crypt
    if (mGradientType==OFFSET_LINEAR)
    {
        double crypt_height = mpCancerParams->GetCryptLength();
        //std::cout << "Crypt Height = " << crypt_height << " height = " << height << "\n";
        double top_of_gradient = 1.0/3.0; // of crypt height.
        if ((height >= -1e-9) && (height < top_of_gradient*crypt_height))
        {
            wnt_level = 1.0 - height/(top_of_gradient*crypt_height);
        }
        else
        {
            wnt_level = 0.0;
        }
    }
    
    assert(wnt_level >= 0.0);
    
    return wnt_level;
}

/**
 * This allows the TissueSimulation to ask whether a WntGradient has been set up or not
 * To let it know whether it should move stem cells around!!
 * 
 * @return result  True if the wnt gradient is set up.
 */
bool SingletonWntGradient::IsGradientSetUp()
{
    bool result = false;
    if (mTypeSet && mpCrypt!=NULL)
    {
        result = true;
    }
    return result;   
}

