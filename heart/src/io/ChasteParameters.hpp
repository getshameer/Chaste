// Copyright (C) 2005-2007 Code Synthesis Tools CC
//
// This program was generated by XML Schema Definition Compiler (XSD)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
//
// In addition, as a special exception, Code Synthesis Tools CC gives
// permission to link this program with the Xerces-C++ library (or with
// modified versions of Xerces-C++ that use the same license as Xerces-C++),
// and distribute linked combinations including the two. You must obey
// the GNU General Public License version 2 in all respects for all of
// the code used other than Xerces-C++. If you modify this copy of the
// program, you may extend this exception to your version of the program,
// but you are not obligated to do so. If you do not wish to do so, delete
// this exception statement from your version.
//
// Furthermore, Code Synthesis Tools CC makes a special exception for
// the Free/Libre and Open Source Software (FLOSS) which is described
// in the accompanying FLOSSE file.
//

#ifndef CHASTE_PARAMETERS_HPP
#define CHASTE_PARAMETERS_HPP

#include <xsd/cxx/version.hxx>

#if (XSD_INT_VERSION != 2030100L)
#error XSD runtime version mismatch
#endif

// Begin prologue.
//
//
// End prologue.

#include <xsd/cxx/pre.hxx>

#ifndef XSD_USE_CHAR
#define XSD_USE_CHAR
#endif

#ifndef XSD_CXX_TREE_USE_CHAR
#define XSD_CXX_TREE_USE_CHAR
#endif

#include <xsd/cxx/tree/exceptions.hxx>
#include <xsd/cxx/tree/elements.hxx>
#include <xsd/cxx/tree/types.hxx>
#include <xsd/cxx/xml/error-handler.hxx>

#include <xsd/cxx/tree/parsing.hxx>

namespace xml_schema
{
  // anyType and anySimpleType.
  //
  typedef ::xsd::cxx::tree::type type;
  typedef ::xsd::cxx::tree::simple_type<type> simple_type;

  // 8-bit
  //
  typedef signed char byte;
  typedef unsigned char unsigned_byte;

  // 16-bit
  //
  typedef short short_;
  typedef unsigned short unsigned_short;

  // 32-bit
  //
  typedef int int_;
  typedef unsigned int unsigned_int;

  // 64-bit
  //
  typedef long long long_;
  typedef unsigned long long unsigned_long;

  // Supposed to be arbitrary-length integral types.
  //
  typedef long long integer;
  typedef integer non_positive_integer;
  typedef integer non_negative_integer;
  typedef integer positive_integer;
  typedef integer negative_integer;

  // Boolean.
  //
  typedef bool boolean;

  // Floating-point types.
  //
  typedef float float_;
  typedef double double_;
  typedef long double decimal;

  // String types.
  //
  typedef ::xsd::cxx::tree::string< char, simple_type > string;
  typedef ::xsd::cxx::tree::normalized_string< char, string > normalized_string;
  typedef ::xsd::cxx::tree::token< char, normalized_string > token;
  typedef ::xsd::cxx::tree::name< char, token > name;
  typedef ::xsd::cxx::tree::nmtoken< char, token > nmtoken;
  typedef ::xsd::cxx::tree::nmtokens< char, simple_type, nmtoken> nmtokens;
  typedef ::xsd::cxx::tree::ncname< char, name > ncname;
  typedef ::xsd::cxx::tree::language< char, token > language;

  // ID/IDREF.
  //
  typedef ::xsd::cxx::tree::id< char, ncname > id;
  typedef ::xsd::cxx::tree::idref< type, char, ncname > idref;
  typedef ::xsd::cxx::tree::idrefs< char, simple_type, idref > idrefs;

  // URI.
  //
  typedef ::xsd::cxx::tree::uri< char, simple_type > uri;

  // Qualified name.
  //
  typedef ::xsd::cxx::tree::qname< char, simple_type, uri, ncname > qname;

  // Binary.
  //
  typedef ::xsd::cxx::tree::buffer< char > buffer;
  typedef ::xsd::cxx::tree::base64_binary< char, simple_type > base64_binary;
  typedef ::xsd::cxx::tree::hex_binary< char, simple_type > hex_binary;

  // Date/time.
  //
  typedef ::xsd::cxx::tree::date< char, simple_type > date;
  typedef ::xsd::cxx::tree::date_time< char, simple_type > date_time;
  typedef ::xsd::cxx::tree::duration< char, simple_type > duration;
  typedef ::xsd::cxx::tree::day< char, simple_type > day;
  typedef ::xsd::cxx::tree::month< char, simple_type > month;
  typedef ::xsd::cxx::tree::month_day< char, simple_type > month_day;
  typedef ::xsd::cxx::tree::year< char, simple_type > year;
  typedef ::xsd::cxx::tree::year_month< char, simple_type > year_month;
  typedef ::xsd::cxx::tree::time< char, simple_type > time;

  // Entity.
  //
  typedef ::xsd::cxx::tree::entity< char, ncname > entity;
  typedef ::xsd::cxx::tree::entities< char, simple_type, entity > entities;

  // Exceptions.
  //
  typedef ::xsd::cxx::tree::exception< char > exception;
  typedef ::xsd::cxx::tree::parsing< char > parsing;
  typedef ::xsd::cxx::tree::expected_element< char > expected_element;
  typedef ::xsd::cxx::tree::unexpected_element< char > unexpected_element;
  typedef ::xsd::cxx::tree::expected_attribute< char > expected_attribute;
  typedef ::xsd::cxx::tree::unexpected_enumerator< char > unexpected_enumerator;
  typedef ::xsd::cxx::tree::expected_text_content< char > expected_text_content;
  typedef ::xsd::cxx::tree::no_type_info< char > no_type_info;
  typedef ::xsd::cxx::tree::not_derived< char > not_derived;
  typedef ::xsd::cxx::tree::duplicate_id< char > duplicate_id;
  typedef ::xsd::cxx::tree::serialization< char > serialization;
  typedef ::xsd::cxx::tree::no_namespace_mapping< char > no_namespace_mapping;
  typedef ::xsd::cxx::tree::no_prefix_mapping< char > no_prefix_mapping;
  typedef ::xsd::cxx::tree::xsi_already_in_use< char > xsi_already_in_use;
  typedef ::xsd::cxx::tree::bounds< char > bounds;

  // Parsing/serialization error.
  //
  typedef ::xsd::cxx::tree::error< char > error;
  typedef ::xsd::cxx::tree::errors< char > errors;

  // Error handler interface.
  //
  typedef ::xsd::cxx::xml::error_handler< char > error_handler;

  // Flags and properties.
  //
  typedef ::xsd::cxx::tree::flags flags;
  typedef ::xsd::cxx::tree::properties< char > properties;

  // DOM user data key for back pointers to tree nodes.
  //
#ifndef XSD_CXX_TREE_TREE_NODE_KEY_IN___XML_SCHEMA
#define XSD_CXX_TREE_TREE_NODE_KEY_IN___XML_SCHEMA

  const XMLCh* const tree_node_key = ::xsd::cxx::tree::user_data_keys::node;

#endif
}

// Forward declarations.
//
class domain_type;
class ionic_model_type;
class point_type;
class box_type;
class stimulus_type;
class chaste_parameters_type;

#include <memory>    // std::auto_ptr
#include <algorithm> // std::binary_search

#include <xsd/cxx/tree/exceptions.hxx>
#include <xsd/cxx/tree/elements.hxx>
#include <xsd/cxx/tree/containers.hxx>
#include <xsd/cxx/tree/list.hxx>

class domain_type: public ::xml_schema::string
{
  public:
  enum _xsd_domain_type
  {
    Mono,
    Bi
  };

  domain_type (_xsd_domain_type);

  domain_type (const ::xml_schema::string&);

  domain_type (const ::xercesc::DOMElement&,
               ::xml_schema::flags = 0,
               ::xml_schema::type* = 0);

  domain_type (const ::xercesc::DOMAttr&,
               ::xml_schema::flags = 0,
               ::xml_schema::type* = 0);

  domain_type (const ::std::basic_string< char >&,
               const ::xercesc::DOMElement*,
               ::xml_schema::flags = 0,
               ::xml_schema::type* = 0);

  domain_type (const domain_type&,
               ::xml_schema::flags = 0,
               ::xml_schema::type* = 0);

  virtual domain_type*
  _clone (::xml_schema::flags = 0,
          ::xml_schema::type* = 0) const;

  domain_type&
  operator= (_xsd_domain_type);

  virtual
  operator _xsd_domain_type () const
  {
    return _xsd_domain_type_convert ();
  }

  protected:
  _xsd_domain_type
  _xsd_domain_type_convert () const;

  public:
  static const char* const _xsd_domain_type_literals_[2];
  static const _xsd_domain_type _xsd_domain_type_indexes_[2];
};

class ionic_model_type: public ::xml_schema::string
{
  public:
  enum _xsd_ionic_model_type
  {
    BackwardEulerFoxModel2002Modified,
    BackwardEulerLuoRudyIModel1991,
    LuoRudyIModel1991OdeSystem,
    FaberRudy2000Version3Optimised,
    FaberRudy2000Version3
  };

  ionic_model_type (_xsd_ionic_model_type);

  ionic_model_type (const ::xml_schema::string&);

  ionic_model_type (const ::xercesc::DOMElement&,
                    ::xml_schema::flags = 0,
                    ::xml_schema::type* = 0);

  ionic_model_type (const ::xercesc::DOMAttr&,
                    ::xml_schema::flags = 0,
                    ::xml_schema::type* = 0);

  ionic_model_type (const ::std::basic_string< char >&,
                    const ::xercesc::DOMElement*,
                    ::xml_schema::flags = 0,
                    ::xml_schema::type* = 0);

  ionic_model_type (const ionic_model_type&,
                    ::xml_schema::flags = 0,
                    ::xml_schema::type* = 0);

  virtual ionic_model_type*
  _clone (::xml_schema::flags = 0,
          ::xml_schema::type* = 0) const;

  ionic_model_type&
  operator= (_xsd_ionic_model_type);

  virtual
  operator _xsd_ionic_model_type () const
  {
    return _xsd_ionic_model_type_convert ();
  }

  protected:
  _xsd_ionic_model_type
  _xsd_ionic_model_type_convert () const;

  public:
  static const char* const _xsd_ionic_model_type_literals_[5];
  static const _xsd_ionic_model_type _xsd_ionic_model_type_indexes_[5];
};

class point_type: public ::xml_schema::type
{
  public:

  struct _xsd_point_type
  {
    typedef ::xml_schema::type base_;
  };

  // X
  // 
  public:
  struct X
  {
    typedef ::xml_schema::double_ type;
    typedef ::xsd::cxx::tree::traits< type, char > traits;
  };

  const X::type&
  X () const;

  X::type&
  X ();

  void
  X (const X::type&);

  // Y
  // 
  public:
  struct Y
  {
    typedef ::xml_schema::double_ type;
    typedef ::xsd::cxx::tree::traits< type, char > traits;
    typedef ::xsd::cxx::tree::optional< type > container;
  };

  const Y::container&
  Y () const;

  Y::container&
  Y ();

  void
  Y (const Y::type&);

  void
  Y (const Y::container&);

  // Z
  // 
  public:
  struct Z
  {
    typedef ::xml_schema::double_ type;
    typedef ::xsd::cxx::tree::traits< type, char > traits;
    typedef ::xsd::cxx::tree::optional< type > container;
  };

  const Z::container&
  Z () const;

  Z::container&
  Z ();

  void
  Z (const Z::type&);

  void
  Z (const Z::container&);

  // Constructors.
  //
  public:
  point_type (const X::type&);

  point_type (const ::xercesc::DOMElement&,
              ::xml_schema::flags = 0,
              ::xml_schema::type* = 0);

  point_type (const point_type&,
              ::xml_schema::flags = 0,
              ::xml_schema::type* = 0);

  virtual point_type*
  _clone (::xml_schema::flags = 0,
          ::xml_schema::type* = 0) const;

  // Implementation.
  //
  private:
  void
  parse (const ::xercesc::DOMElement&, ::xml_schema::flags);

  ::xsd::cxx::tree::one< X::type > _xsd_X_;
  ::xsd::cxx::tree::optional< Y::type > _xsd_Y_;
  ::xsd::cxx::tree::optional< Z::type > _xsd_Z_;
};

class box_type: public ::xml_schema::type
{
  public:

  struct _xsd_box_type
  {
    typedef ::xml_schema::type base_;
  };

  // CornerA
  // 
  public:
  struct CornerA
  {
    typedef ::point_type type;
    typedef ::xsd::cxx::tree::traits< type, char > traits;
  };

  const CornerA::type&
  CornerA () const;

  CornerA::type&
  CornerA ();

  void
  CornerA (const CornerA::type&);

  void
  CornerA (::std::auto_ptr< CornerA::type >);

  // CornerB
  // 
  public:
  struct CornerB
  {
    typedef ::point_type type;
    typedef ::xsd::cxx::tree::traits< type, char > traits;
  };

  const CornerB::type&
  CornerB () const;

  CornerB::type&
  CornerB ();

  void
  CornerB (const CornerB::type&);

  void
  CornerB (::std::auto_ptr< CornerB::type >);

  // Constructors.
  //
  public:
  box_type (const CornerA::type&,
            const CornerB::type&);

  box_type (const ::xercesc::DOMElement&,
            ::xml_schema::flags = 0,
            ::xml_schema::type* = 0);

  box_type (const box_type&,
            ::xml_schema::flags = 0,
            ::xml_schema::type* = 0);

  virtual box_type*
  _clone (::xml_schema::flags = 0,
          ::xml_schema::type* = 0) const;

  // Implementation.
  //
  private:
  void
  parse (const ::xercesc::DOMElement&, ::xml_schema::flags);

  ::xsd::cxx::tree::one< CornerA::type > _xsd_CornerA_;
  ::xsd::cxx::tree::one< CornerB::type > _xsd_CornerB_;
};

class stimulus_type: public ::xml_schema::type
{
  public:

  struct _xsd_stimulus_type
  {
    typedef ::xml_schema::type base_;
  };

  // Strength
  // 
  public:
  struct Strength
  {
    typedef ::xml_schema::double_ type;
    typedef ::xsd::cxx::tree::traits< type, char > traits;
  };

  const Strength::type&
  Strength () const;

  Strength::type&
  Strength ();

  void
  Strength (const Strength::type&);

  // Duration
  // 
  public:
  struct Duration
  {
    typedef ::xml_schema::double_ type;
    typedef ::xsd::cxx::tree::traits< type, char > traits;
  };

  const Duration::type&
  Duration () const;

  Duration::type&
  Duration ();

  void
  Duration (const Duration::type&);

  // Delay
  // 
  public:
  struct Delay
  {
    typedef ::xml_schema::double_ type;
    typedef ::xsd::cxx::tree::traits< type, char > traits;
  };

  const Delay::type&
  Delay () const;

  Delay::type&
  Delay ();

  void
  Delay (const Delay::type&);

  // Location
  // 
  public:
  struct Location
  {
    typedef ::box_type type;
    typedef ::xsd::cxx::tree::traits< type, char > traits;
  };

  const Location::type&
  Location () const;

  Location::type&
  Location ();

  void
  Location (const Location::type&);

  void
  Location (::std::auto_ptr< Location::type >);

  // Constructors.
  //
  public:
  stimulus_type (const Strength::type&,
                 const Duration::type&,
                 const Delay::type&,
                 const Location::type&);

  stimulus_type (const ::xercesc::DOMElement&,
                 ::xml_schema::flags = 0,
                 ::xml_schema::type* = 0);

  stimulus_type (const stimulus_type&,
                 ::xml_schema::flags = 0,
                 ::xml_schema::type* = 0);

  virtual stimulus_type*
  _clone (::xml_schema::flags = 0,
          ::xml_schema::type* = 0) const;

  // Implementation.
  //
  private:
  void
  parse (const ::xercesc::DOMElement&, ::xml_schema::flags);

  ::xsd::cxx::tree::one< Strength::type > _xsd_Strength_;
  ::xsd::cxx::tree::one< Duration::type > _xsd_Duration_;
  ::xsd::cxx::tree::one< Delay::type > _xsd_Delay_;
  ::xsd::cxx::tree::one< Location::type > _xsd_Location_;
};

class chaste_parameters_type: public ::xml_schema::type
{
  public:

  struct _xsd_chaste_parameters_type
  {
    typedef ::xml_schema::type base_;
  };

  // SimulationDuration
  // 
  public:
  struct SimulationDuration
  {
    typedef ::xml_schema::decimal type;
    typedef ::xsd::cxx::tree::traits< type, char > traits;
  };

  const SimulationDuration::type&
  SimulationDuration () const;

  SimulationDuration::type&
  SimulationDuration ();

  void
  SimulationDuration (const SimulationDuration::type&);

  // Domain
  // 
  public:
  struct Domain
  {
    typedef ::domain_type type;
    typedef ::xsd::cxx::tree::traits< type, char > traits;
  };

  const Domain::type&
  Domain () const;

  Domain::type&
  Domain ();

  void
  Domain (const Domain::type&);

  void
  Domain (::std::auto_ptr< Domain::type >);

  // IonicModel
  // 
  public:
  struct IonicModel
  {
    typedef ::ionic_model_type type;
    typedef ::xsd::cxx::tree::traits< type, char > traits;
  };

  const IonicModel::type&
  IonicModel () const;

  IonicModel::type&
  IonicModel ();

  void
  IonicModel (const IonicModel::type&);

  void
  IonicModel (::std::auto_ptr< IonicModel::type >);

  // SlabWidth
  // 
  public:
  struct SlabWidth
  {
    typedef ::xml_schema::decimal type;
    typedef ::xsd::cxx::tree::traits< type, char > traits;
  };

  const SlabWidth::type&
  SlabWidth () const;

  SlabWidth::type&
  SlabWidth ();

  void
  SlabWidth (const SlabWidth::type&);

  // SlabHeight
  // 
  public:
  struct SlabHeight
  {
    typedef ::xml_schema::decimal type;
    typedef ::xsd::cxx::tree::traits< type, char > traits;
  };

  const SlabHeight::type&
  SlabHeight () const;

  SlabHeight::type&
  SlabHeight ();

  void
  SlabHeight (const SlabHeight::type&);

  // InterNodeSpace
  // 
  public:
  struct InterNodeSpace
  {
    typedef ::xml_schema::decimal type;
    typedef ::xsd::cxx::tree::traits< type, char > traits;
  };

  const InterNodeSpace::type&
  InterNodeSpace () const;

  InterNodeSpace::type&
  InterNodeSpace ();

  void
  InterNodeSpace (const InterNodeSpace::type&);

  // Stimulus
  // 
  public:
  struct Stimulus
  {
    typedef ::stimulus_type type;
    typedef ::xsd::cxx::tree::traits< type, char > traits;
    typedef ::xsd::cxx::tree::sequence< type > container;
    typedef container::iterator iterator;
    typedef container::const_iterator const_iterator;
  };

  const Stimulus::container&
  Stimulus () const;

  Stimulus::container&
  Stimulus ();

  void
  Stimulus (const Stimulus::container&);

  // OutputDirectory
  // 
  public:
  struct OutputDirectory
  {
    typedef ::xml_schema::string type;
    typedef ::xsd::cxx::tree::traits< type, char > traits;
  };

  const OutputDirectory::type&
  OutputDirectory () const;

  OutputDirectory::type&
  OutputDirectory ();

  void
  OutputDirectory (const OutputDirectory::type&);

  void
  OutputDirectory (::std::auto_ptr< OutputDirectory::type >);

  // MeshOutputDirectory
  // 
  public:
  struct MeshOutputDirectory
  {
    typedef ::xml_schema::string type;
    typedef ::xsd::cxx::tree::traits< type, char > traits;
  };

  const MeshOutputDirectory::type&
  MeshOutputDirectory () const;

  MeshOutputDirectory::type&
  MeshOutputDirectory ();

  void
  MeshOutputDirectory (const MeshOutputDirectory::type&);

  void
  MeshOutputDirectory (::std::auto_ptr< MeshOutputDirectory::type >);

  // Constructors.
  //
  public:
  chaste_parameters_type (const SimulationDuration::type&,
                          const Domain::type&,
                          const IonicModel::type&,
                          const SlabWidth::type&,
                          const SlabHeight::type&,
                          const InterNodeSpace::type&,
                          const OutputDirectory::type&,
                          const MeshOutputDirectory::type&);

  chaste_parameters_type (const ::xercesc::DOMElement&,
                          ::xml_schema::flags = 0,
                          ::xml_schema::type* = 0);

  chaste_parameters_type (const chaste_parameters_type&,
                          ::xml_schema::flags = 0,
                          ::xml_schema::type* = 0);

  virtual chaste_parameters_type*
  _clone (::xml_schema::flags = 0,
          ::xml_schema::type* = 0) const;

  // Implementation.
  //
  private:
  void
  parse (const ::xercesc::DOMElement&, ::xml_schema::flags);

  ::xsd::cxx::tree::one< SimulationDuration::type > _xsd_SimulationDuration_;
  ::xsd::cxx::tree::one< Domain::type > _xsd_Domain_;
  ::xsd::cxx::tree::one< IonicModel::type > _xsd_IonicModel_;
  ::xsd::cxx::tree::one< SlabWidth::type > _xsd_SlabWidth_;
  ::xsd::cxx::tree::one< SlabHeight::type > _xsd_SlabHeight_;
  ::xsd::cxx::tree::one< InterNodeSpace::type > _xsd_InterNodeSpace_;
  ::xsd::cxx::tree::sequence< Stimulus::type > _xsd_Stimulus_;
  ::xsd::cxx::tree::one< OutputDirectory::type > _xsd_OutputDirectory_;
  ::xsd::cxx::tree::one< MeshOutputDirectory::type > _xsd_MeshOutputDirectory_;
};

#include <iosfwd>

#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMInputSource.hpp>
#include <xercesc/dom/DOMErrorHandler.hpp>

// Read from a URI or a local file.
//

::std::auto_ptr< ::chaste_parameters_type >
ChasteParameters (const ::std::basic_string< char >&,
                  ::xml_schema::flags = 0,
                  const ::xsd::cxx::tree::properties< char >& = ::xsd::cxx::tree::properties< char > ());

::std::auto_ptr< ::chaste_parameters_type >
ChasteParameters (const ::std::basic_string< char >&,
                  ::xsd::cxx::xml::error_handler< char >&,
                  ::xml_schema::flags = 0,
                  const ::xsd::cxx::tree::properties< char >& = ::xsd::cxx::tree::properties< char > ());

::std::auto_ptr< ::chaste_parameters_type >
ChasteParameters (const ::std::basic_string< char >&,
                  ::xercesc::DOMErrorHandler&,
                  ::xml_schema::flags = 0,
                  const ::xsd::cxx::tree::properties< char >& = ::xsd::cxx::tree::properties< char > ());


// Read from std::istream.
//

::std::auto_ptr< ::chaste_parameters_type >
ChasteParameters (::std::istream&,
                  ::xml_schema::flags = 0,
                  const ::xsd::cxx::tree::properties< char >& = ::xsd::cxx::tree::properties< char > ());

::std::auto_ptr< ::chaste_parameters_type >
ChasteParameters (::std::istream&,
                  ::xsd::cxx::xml::error_handler< char >&,
                  ::xml_schema::flags = 0,
                  const ::xsd::cxx::tree::properties< char >& = ::xsd::cxx::tree::properties< char > ());

::std::auto_ptr< ::chaste_parameters_type >
ChasteParameters (::std::istream&,
                  ::xercesc::DOMErrorHandler&,
                  ::xml_schema::flags = 0,
                  const ::xsd::cxx::tree::properties< char >& = ::xsd::cxx::tree::properties< char > ());


::std::auto_ptr< ::chaste_parameters_type >
ChasteParameters (::std::istream&,
                  const ::std::basic_string< char >& id,
                  ::xml_schema::flags = 0,
                  const ::xsd::cxx::tree::properties< char >& = ::xsd::cxx::tree::properties< char > ());

::std::auto_ptr< ::chaste_parameters_type >
ChasteParameters (::std::istream&,
                  const ::std::basic_string< char >& id,
                  ::xsd::cxx::xml::error_handler< char >&,
                  ::xml_schema::flags = 0,
                  const ::xsd::cxx::tree::properties< char >& = ::xsd::cxx::tree::properties< char > ());

::std::auto_ptr< ::chaste_parameters_type >
ChasteParameters (::std::istream&,
                  const ::std::basic_string< char >& id,
                  ::xercesc::DOMErrorHandler&,
                  ::xml_schema::flags = 0,
                  const ::xsd::cxx::tree::properties< char >& = ::xsd::cxx::tree::properties< char > ());


// Read from InputSource.
//

::std::auto_ptr< ::chaste_parameters_type >
ChasteParameters (const ::xercesc::DOMInputSource&,
                  ::xml_schema::flags = 0,
                  const ::xsd::cxx::tree::properties< char >& = ::xsd::cxx::tree::properties< char > ());

::std::auto_ptr< ::chaste_parameters_type >
ChasteParameters (const ::xercesc::DOMInputSource&,
                  ::xsd::cxx::xml::error_handler< char >&,
                  ::xml_schema::flags = 0,
                  const ::xsd::cxx::tree::properties< char >& = ::xsd::cxx::tree::properties< char > ());

::std::auto_ptr< ::chaste_parameters_type >
ChasteParameters (const ::xercesc::DOMInputSource&,
                  ::xercesc::DOMErrorHandler&,
                  ::xml_schema::flags = 0,
                  const ::xsd::cxx::tree::properties< char >& = ::xsd::cxx::tree::properties< char > ());


// Read from DOM.
//

::std::auto_ptr< ::chaste_parameters_type >
ChasteParameters (const ::xercesc::DOMDocument&,
                  ::xml_schema::flags = 0,
                  const ::xsd::cxx::tree::properties< char >& = ::xsd::cxx::tree::properties< char > ());


#include <xsd/cxx/post.hxx>

// Begin epilogue.
//
//
// End epilogue.

#endif // CHASTE_PARAMETERS_HPP
