#ifndef _TESTCOLUMNDATAREADERWRITER_HPP_
#define _TESTCOLUMNDATAREADERWRITER_HPP_

//  MyTestSuite.h
#include <cxxtest/TestSuite.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include "ColumnDataWriter.hpp"
#include "ColumnDataReader.hpp"
#include "global/src/Exception.hpp"

using namespace std;
          
class TestColumnDataReaderWriter : public CxxTest::TestSuite 
{

private: 
	ColumnDataWriter *mpTestWriter;
	ColumnDataReader *mpTestReader;
	
	bool filesMatch(std::string testfileName, std::string goodfileName)
	{	
		bool matching = true;
			
		ifstream testfile(testfileName.c_str(),ios::in);
        ifstream goodfile(goodfileName.c_str(),ios::in);
        std::string teststring;
        std::string goodstring;
        
        if (!testfile.is_open() || !goodfile.is_open())
        {
        	throw new Exception("Files not present.");
        }
        
        while(getline(testfile, teststring))
        {
              getline(goodfile,goodstring);
              if (teststring != goodstring)
              {
              		matching = false;
              }
        }
        
        if(getline(goodfile,goodstring))
        {
        	matching = false;
        }
        
        testfile.close();
        goodfile.close();
        return matching;
	}
	
public:

    void setUp()
    {
		//TS_TRACE("Beginning test...");
    }
    void tearDown()
    {
		// TS_TRACE("Completed test");
    }
    void testCreateColumnWriter(void)
    {
        //create a new csvdata writer
        TS_ASSERT_THROWS_NOTHING(mpTestWriter = new ColumnDataWriter("testoutput","test"));
        //check that the output directory exists
        //use the Boost libraries for this check
        
        delete mpTestWriter; 
    }
    
    void testCreateColumnReader(void)
    {
        TS_ASSERT_THROWS_ANYTHING(mpTestReader = new ColumnDataReader("testoutput","testdoesnotexist"));
        TS_ASSERT_THROWS_ANYTHING(mpTestReader = new ColumnDataReader("io/test/data","testbad"));
        delete mpTestReader; 
    }
    
    void testDefineUnlimitedDimension( void )
    {
        TS_ASSERT_THROWS_NOTHING(mpTestWriter = new ColumnDataWriter("testoutput","test"));
        TS_ASSERT_THROWS_NOTHING(mpTestWriter->DefineUnlimitedDimension("Time","msecs"));
        TS_ASSERT_THROWS_ANYTHING(mpTestWriter->DefineUnlimitedDimension("Time","msecs"));

        
        
        TS_ASSERT_THROWS_ANYTHING(mpTestWriter->DefineUnlimitedDimension("Time","m secs"));
        TS_ASSERT_THROWS_ANYTHING(mpTestWriter->DefineUnlimitedDimension("T,i,m,e","msecs"));
        TS_ASSERT_THROWS_ANYTHING(mpTestWriter->DefineUnlimitedDimension("","msecs"));
        
        delete mpTestWriter;
    }
 
    void testDefineFixedDimension( void )
    {
        TS_ASSERT_THROWS_NOTHING(mpTestWriter = new ColumnDataWriter("testoutput","test"));
        TS_ASSERT_THROWS_NOTHING(mpTestWriter->DefineFixedDimension("Node","dimensionless", 5000));
 
        TS_ASSERT_THROWS_ANYTHING(mpTestWriter->DefineFixedDimension("Node ","dimensionless", 5000));
        TS_ASSERT_THROWS_ANYTHING(mpTestWriter->DefineFixedDimension("Node","dimension.less", 5000));
        TS_ASSERT_THROWS_ANYTHING(mpTestWriter->DefineFixedDimension("*Node*","dimensionless", 5000));
        
        delete mpTestWriter;
    }
    
    void testDefineVariable( void )
    {
        TS_ASSERT_THROWS_NOTHING(mpTestWriter = new ColumnDataWriter("testoutput","test"));
        int ina_var_id = 0;
        int ik_var_id = 0;

        TS_ASSERT_THROWS_NOTHING(mpTestWriter->DefineUnlimitedDimension("Time","msecs"));

        TS_ASSERT_THROWS_NOTHING(ina_var_id = mpTestWriter->DefineVariable("I_Na","milliamperes"));
        TS_ASSERT_THROWS_NOTHING(ik_var_id = mpTestWriter->DefineVariable("I_K","milliamperes"));
        TS_ASSERT_THROWS_NOTHING(mpTestWriter->DefineVariable("Dummy",""));
        
        // Bad variable names/units
        TS_ASSERT_THROWS_ANYTHING(ik_var_id = mpTestWriter->DefineVariable("I_K","milli amperes"));
        TS_ASSERT_THROWS_ANYTHING(ik_var_id = mpTestWriter->DefineVariable("I   K","milliamperes"));
        TS_ASSERT_THROWS_ANYTHING(ik_var_id = mpTestWriter->DefineVariable("I.K","milliamperes"));
        
        TS_ASSERT_EQUALS(ina_var_id, 0);
        TS_ASSERT_EQUALS(ik_var_id, 1);

		delete mpTestWriter;
    }

    void testEndDefineMode( void )
    {
        TS_ASSERT_THROWS_NOTHING(mpTestWriter = new ColumnDataWriter("testoutput","testdefine"));
        //ending define mode without having defined a dimension and a variable should raise an exception
        TS_ASSERT_THROWS_ANYTHING(mpTestWriter->EndDefineMode());
        
        int ina_var_id = 0;
        int ik_var_id = 0;

        TS_ASSERT_THROWS_NOTHING(mpTestWriter->DefineUnlimitedDimension("Time","msecs"));
        TS_ASSERT_THROWS_ANYTHING(mpTestWriter->EndDefineMode());

        TS_ASSERT_THROWS_NOTHING(ina_var_id = mpTestWriter->DefineVariable("I_Na","milliamperes"));
        TS_ASSERT_THROWS_NOTHING(ik_var_id = mpTestWriter->DefineVariable("I_K","milliamperes"));
        TS_ASSERT_THROWS_NOTHING(mpTestWriter->EndDefineMode());
        TS_ASSERT_THROWS_ANYTHING(mpTestWriter->DefineVariable("I_Ca","milli amperes")); 
        TS_ASSERT_THROWS_ANYTHING(mpTestWriter->DefineUnlimitedDimension("Time","msecs"));
        TS_ASSERT_THROWS_ANYTHING(mpTestWriter->DefineFixedDimension("Node","dimensionless", 5000));
        
        delete mpTestWriter;

		TS_ASSERT(filesMatch("testoutput/testdefine.dat", 
		                     "io/test/data/testdefine_good.dat"));
		                     
		TS_ASSERT(filesMatch("testoutput/testdefine.info", 
		                     "io/test/data/testdefine_good.info"));
		                     
		TS_ASSERT(!filesMatch("testoutput/testdefine.info", 
		                      "io/test/data/testdefine_bad.info"));
        
        
    }

    void testPutVariableInUnlimitedFile( void )
    {
        TS_ASSERT_THROWS_NOTHING(mpTestWriter = new ColumnDataWriter("testoutput","testunlimited"));
        int time_var_id = 0;
        int ina_var_id = 0;
        int ik_var_id = 0;
        int ica_var_id = 0;
        TS_ASSERT_THROWS_NOTHING(time_var_id = mpTestWriter->DefineUnlimitedDimension("Time","msecs"));
        TS_ASSERT_THROWS_NOTHING(ina_var_id = mpTestWriter->DefineVariable("I_Na","milliamperes"));
        TS_ASSERT_THROWS_NOTHING(ik_var_id = mpTestWriter->DefineVariable("I_K","milliamperes"));        
        TS_ASSERT_THROWS_NOTHING(ica_var_id = mpTestWriter->DefineVariable("I_Ca","milliamperes"));
        TS_ASSERT_THROWS_NOTHING(mpTestWriter->EndDefineMode());

        
        for (int i=0; i<10; i++)
        {
        	mpTestWriter->PutVariable(time_var_id, (double)(i)/10);
        	mpTestWriter->PutVariable(ina_var_id, 12.0);
        	mpTestWriter->PutVariable(ica_var_id, ((double)((i+1)*(i+1)))/3.0);
        	mpTestWriter->PutVariable(ik_var_id, 7124.12355553*((double)(i+1))/12.0);
        	mpTestWriter->AdvanceAlongUnlimitedDimension();
        }
        
        int i = 10;
        mpTestWriter->PutVariable(time_var_id, (double)(i)/10);
    	mpTestWriter->PutVariable(ina_var_id, 12.0);
    	mpTestWriter->PutVariable(ica_var_id, ((double)((i+1)*(i+1)))/3.0);
    	mpTestWriter->PutVariable(ik_var_id, 7124.12355553*((double)(i+1))/12.0);
        
		delete mpTestWriter;

		TS_ASSERT(filesMatch("testoutput/testunlimited.dat", 
		                     "io/test/data/testunlimited_good.dat"));
		                     
		TS_ASSERT_THROWS_NOTHING(mpTestReader = new ColumnDataReader("testoutput","testunlimited"));
		
		//TS_ASSERT_THROWS_ANYTHING(std::vector<double> values_ik = mpTestReader->GetValues("I_K",3));

		std::vector<double> values_ik = mpTestReader->GetValues("I_K");
		
		for (int i=0; i<11; i++)
		{
			TS_ASSERT_DELTA(values_ik[i]/(7124.12355553*((double)(i+1))/12.0), 1.0, 1e-3);
		}		

        std::vector<double> time_values = mpTestReader->GetUnlimitedDimensionValues();
        for(int i=0; i < 10; i++)
        {
            TS_ASSERT_DELTA(time_values[i],i*0.1,1e-3);   
        }
        delete mpTestReader;		                     
		                     
    }
    
    
    void testPutVariableInUnlimitedNegativeFile( void )
    {
        TS_ASSERT_THROWS_NOTHING(mpTestWriter = new ColumnDataWriter("testoutput","testunlimitednegative"));
        int time_var_id = 0;
        int ina_var_id = 0;
        int ik_var_id = 0;
        int ica_var_id = 0;
        TS_ASSERT_THROWS_NOTHING(time_var_id = mpTestWriter->DefineUnlimitedDimension("Time","msecs"));
        TS_ASSERT_THROWS_NOTHING(ina_var_id = mpTestWriter->DefineVariable("I_Na","milliamperes"));
        TS_ASSERT_THROWS_NOTHING(ik_var_id = mpTestWriter->DefineVariable("I_K","milliamperes"));
        TS_ASSERT_THROWS_ANYTHING(time_var_id = mpTestWriter->DefineVariable("Time","msecs"));
        TS_ASSERT_THROWS_NOTHING(ica_var_id = mpTestWriter->DefineVariable("I_Ca","milliamperes"));
        //TS_TRACE("Here");
        TS_ASSERT_THROWS_NOTHING(mpTestWriter->EndDefineMode());
        int i = 12;

        TS_ASSERT_THROWS_NOTHING(mpTestWriter->PutVariable(time_var_id, 0.2));
        TS_ASSERT_THROWS_NOTHING(mpTestWriter->PutVariable(ina_var_id, (double) -i));
        TS_ASSERT_THROWS_NOTHING(mpTestWriter->PutVariable(ica_var_id, 33.124));
        TS_ASSERT_THROWS_NOTHING(mpTestWriter->PutVariable(ik_var_id, 7124.12355553));
        mpTestWriter->AdvanceAlongUnlimitedDimension();
        TS_ASSERT_THROWS_NOTHING(mpTestWriter->PutVariable(ica_var_id, -33.124));
    
    	delete mpTestWriter; 

		TS_ASSERT(filesMatch("testoutput/testunlimitednegative.dat", 
		                     "io/test/data/testunlimitednegative_good.dat"));
		                      
    }

    void testPutVariableInFixedFile( void )
    {

        TS_ASSERT_THROWS_NOTHING(mpTestWriter = new ColumnDataWriter("testoutput","testfixed"));
        int node_var_id = 0;
        int ina_var_id = 0;
        int ik_var_id = 0;
        int ica_var_id = 0;
        TS_ASSERT_THROWS_NOTHING(node_var_id = mpTestWriter->DefineFixedDimension("Node","dimensionless",4));
        TS_ASSERT_THROWS_NOTHING(ina_var_id = mpTestWriter->DefineVariable("I_Na","milliamperes"));
        TS_ASSERT_THROWS_NOTHING(ik_var_id = mpTestWriter->DefineVariable("I_K","milliamperes"));
        TS_ASSERT_THROWS_ANYTHING(node_var_id = mpTestWriter->DefineVariable("Node","dimensionless")) ;
        TS_ASSERT_THROWS_NOTHING(ica_var_id = mpTestWriter->DefineVariable("I_Ca","milliamperes"));
        TS_ASSERT_THROWS_NOTHING(mpTestWriter->EndDefineMode());
                
        for (int i=0; i<4; i++)
        {
        	mpTestWriter->PutVariable(node_var_id, (double)(i+1), i);
        	mpTestWriter->PutVariable(ina_var_id, 12.0, i);
        	mpTestWriter->PutVariable(ica_var_id, ((double)((i+1)*(i+1)))/3.0, i);
        	mpTestWriter->PutVariable(ik_var_id, 7124.12355553*((double)(i+1))/12.0, i);
        }

//        mpTestWriter->PutVariable(ica_var_id, 33.124,3);
//        mpTestWriter->PutVariable(ik_var_id, 7124.12355553,3);
//        TS_ASSERT_THROWS_ANYTHING(mpTestWriter->AdvanceAlongUnlimitedDimension());
//        mpTestWriter->PutVariable(ica_var_id, 63.124,2);

		delete mpTestWriter;

		TS_ASSERT(filesMatch("testoutput/testfixed.dat", 
		                     "io/test/data/testfixed_good.dat"));
		                     
		TS_ASSERT(filesMatch("testoutput/testfixed.info", 
		                     "io/test/data/testfixed_good.info"));
		                     
		TS_ASSERT_THROWS_NOTHING(mpTestReader = new ColumnDataReader("testoutput","testfixed"));
		
		//TS_ASSERT_THROWS_ANYTHING(std::vector<double> values_ik = mpTestReader->GetValues("I_K"));
		
		for (int i=0; i<4; i++)
		{
			std::vector<double> values_ik = mpTestReader->GetValues("I_K", i);
			TS_ASSERT_DELTA(values_ik[0]/(7124.12355553*((double)(i+1))/12.0), 1.0, 1e-3);
		}
        
		TS_ASSERT_THROWS_ANYTHING(std::vector<double> values_dodgy = mpTestReader->GetValues("non-existent_variable",1));
        delete mpTestReader;
    }
    
    
    void testPutNegativeVariableInFixedFile( void )
    {

        TS_ASSERT_THROWS_NOTHING(mpTestWriter = new ColumnDataWriter("testoutput","testfixed_negatives"));
        int node_var_id = 0;
        int ina_var_id = 0;
        int ik_var_id = 0;
        int ica_var_id = 0;
        TS_ASSERT_THROWS_NOTHING(node_var_id = mpTestWriter->DefineFixedDimension("Node","dimensionless",4));
        TS_ASSERT_THROWS_NOTHING(ina_var_id = mpTestWriter->DefineVariable("I_Na","milliamperes"));
        TS_ASSERT_THROWS_NOTHING(ik_var_id = mpTestWriter->DefineVariable("I_K","milliamperes"));
        TS_ASSERT_THROWS_ANYTHING(node_var_id = mpTestWriter->DefineVariable("Node","dimensionless")) ;
        TS_ASSERT_THROWS_NOTHING(ica_var_id = mpTestWriter->DefineVariable("I_Ca","milliamperes"));
        TS_ASSERT_THROWS_NOTHING(mpTestWriter->EndDefineMode());
        int i = 12;

        mpTestWriter->PutVariable(ina_var_id, (double) i,0);
        mpTestWriter->PutVariable(ina_var_id, (double) -i,1);
        mpTestWriter->PutVariable(ica_var_id, -33.124,3);
        mpTestWriter->PutVariable(ik_var_id, 7124.12355553,3);
        TS_ASSERT_THROWS_ANYTHING(mpTestWriter->AdvanceAlongUnlimitedDimension());
        mpTestWriter->PutVariable(ica_var_id, -63.124,2);
        mpTestWriter->PutVariable(node_var_id, 1,0);
        mpTestWriter->PutVariable(node_var_id, -4,3);
    
    	delete mpTestWriter;
    	
		TS_ASSERT(filesMatch("testoutput/testfixed_negatives.dat", 
		                     "io/test/data/testfixed_negatives_good.dat"));

    }
    
    void testPutVariableInFixedandUnlimitedFile( void )
    {

        TS_ASSERT_THROWS_NOTHING(mpTestWriter = new ColumnDataWriter("testoutput","testfixedandunlimited"));
        int time_var_id = 0;
        int node_var_id = 0;
        int ina_var_id = 0;
        int ik_var_id = 0;
        int ica_var_id = 0;
        TS_ASSERT_THROWS_NOTHING(node_var_id = mpTestWriter->DefineFixedDimension("Node","dimensionless",4));
        TS_ASSERT_THROWS_NOTHING(time_var_id = mpTestWriter->DefineUnlimitedDimension("Time","msecs"));
        TS_ASSERT_THROWS_ANYTHING(time_var_id = mpTestWriter->DefineVariable("Time","msecs"));
        TS_ASSERT_THROWS_NOTHING(ina_var_id = mpTestWriter->DefineVariable("I_Na","milliamperes"));
        TS_ASSERT_THROWS_NOTHING(ik_var_id = mpTestWriter->DefineVariable("I_K","milliamperes"));
        TS_ASSERT_THROWS_NOTHING(ica_var_id = mpTestWriter->DefineVariable("I_Ca","milliamperes"));
        TS_ASSERT_THROWS_NOTHING(mpTestWriter->EndDefineMode());
        int i = 12;

        TS_ASSERT_THROWS_NOTHING(mpTestWriter->PutVariable(time_var_id, 0.1));
        TS_ASSERT_THROWS_NOTHING(mpTestWriter->PutVariable(ina_var_id, (double) i,0));
        TS_ASSERT_THROWS_NOTHING(mpTestWriter->PutVariable(ina_var_id, (double) i,1));
        TS_ASSERT_THROWS_NOTHING(mpTestWriter->PutVariable(ica_var_id, -33.124,3));
        TS_ASSERT_THROWS_NOTHING(mpTestWriter->PutVariable(ik_var_id, 7124.12355553,3));
        TS_ASSERT_THROWS_NOTHING(mpTestWriter->AdvanceAlongUnlimitedDimension());
        TS_ASSERT_THROWS_NOTHING(mpTestWriter->PutVariable(ica_var_id, 63.124,2));
        TS_ASSERT_THROWS_NOTHING(mpTestWriter->PutVariable(ica_var_id, -35.124,3));
        TS_ASSERT_THROWS_NOTHING(mpTestWriter->PutVariable(time_var_id, 0.2));
        TS_ASSERT_THROWS_ANYTHING(mpTestWriter->PutVariable(time_var_id, 0.2,3));

		delete mpTestWriter;
		
		TS_ASSERT(filesMatch("testoutput/testfixedandunlimited_unlimited.dat", 
		                     "io/test/data/testfixedandunlimited_unlimited_good.dat"));

		TS_ASSERT(filesMatch("testoutput/testfixedandunlimited_1.dat", 
		                     "io/test/data/testfixedandunlimited_1_good.dat"));
                             
        TS_ASSERT_THROWS_NOTHING(mpTestReader = new ColumnDataReader("testoutput","testfixedandunlimited"));                             

        std::vector<double> time_values = mpTestReader->GetUnlimitedDimensionValues();
        std::vector<double> ica_values = mpTestReader->GetValues("I_Ca",3);
        for(int i=0; i < 2; i++)
        {
            TS_ASSERT_DELTA(time_values[i],(i+1)*0.1,1e-3);   
            TS_ASSERT_DELTA(ica_values[i],-33.124 - i * 2,1e-3);   
        }
        
                                     
    }
    
};

#endif //_TESTCOLUMNDATAREADERWRITER_HPP_
