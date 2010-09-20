# Configuration

"""Copyright (C) University of Oxford, 2005-2010

University of Oxford means the Chancellor, Masters and Scholars of the
University of Oxford, having an administrative office at Wellington
Square, Oxford OX1 2JD, UK.

This file is part of Chaste.

Chaste is free software: you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published
by the Free Software Foundation, either version 2.1 of the License, or
(at your option) any later version.

Chaste is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details. The offer of Chaste under the terms of the
License is subject to the License being interpreted in accordance with
English Law and subject to any action against the University of Oxford
being under the jurisdiction of the English Courts.

You should have received a copy of the GNU Lesser General Public License
along with Chaste. If not, see <http://www.gnu.org/licenses/>.
"""

import os
import sys

############################################################
# TO CONFIGURE YOUR MACHINE: if you have followed the manual
# installation instructions, edit the definition of
# chaste_libs_path below to point to where you installed the
# dependencies.
############################################################

#EDIT HERE
#For a simple installation all paths will be below this directory
chaste_libs_path = '/home/scratch/chaste-libs/'
#EDIT HERE

if not os.path.exists(chaste_libs_path) or not os.path.isdir(chaste_libs_path):
    print >>sys.stderr, "Chaste dependencies folder", chaste_libs_path, \
        "not found; please edit python/hostconfig/default.py"
    sys.exit(1)

petsc_2_3_path = chaste_libs_path+'petsc-2.3.3-p15/'
petsc_build_name = 'linux-gnu'
petsc_build_name_profile = 'linux-gnu'
petsc_build_name_optimized = 'linux-gnu-opt'
dealii_path = ''
parmetis_path = chaste_libs_path+'/ParMetis-3.1/'
# If you have the Intel compiler installed, set this to the folder where it lives
intel_path = '/opt/intel/cc/9.1.039/'
# You may need to edit this to ensure that the intel compiler finds the right gcc libraries, e.g.
#icpc = 'icpc -gcc-version=410 -I /usr/include/c++/4.1.3/x86_64-linux-gnu/ -I/usr/include/c++/4.1.3/    -I/usr/include/c++/4.1.3/backward'
icpc = 'icpc'

other_includepaths = [chaste_libs_path+'hdf5/include',
                      chaste_libs_path+'xerces/include',
                      chaste_libs_path+'boost/include/boost-1_34_1',
                      chaste_libs_path+'xsd-3.2.0-i686-linux-gnu/libxsd',
                      parmetis_path]

other_libpaths = [chaste_libs_path+'lib',
                  chaste_libs_path+'boost/lib', 
                  chaste_libs_path+'xerces/lib',
                  chaste_libs_path+'hdf5/lib',
                  chaste_libs_path+'rdf/lib',
                  os.path.join(petsc_2_3_path, 'externalpackages/f2cblaslapack/linux-gnu'),
                  parmetis_path]

# The order of libraries in these lists matters!
blas_lapack = ['f2clapack', 'f2cblas'] # Note: Lapack before BLAS
other_libraries = ['boost_serialization', 'xerces-c', 'hdf5', 'z', 'parmetis', 'metis']
  # Note: parmetis before metis, hdf5 before z.
# Note that boost serialization sometimes has a different name:
# other_libraries = ['boost_serialization-gcc41', 'xerces-c', 'hdf5', 'z', 'parmetis', 'metis']

tools = {'mpirun': chaste_libs_path+'mpi/bin/mpirun',
         'mpicxx': chaste_libs_path+'mpi/bin/mpicxx',
         'xsd': chaste_libs_path+'bin/xsd'}


# use_vtk set to false initially. Change to True if VTK development libraries are 
# available.
use_vtk = False

#Extra libraries for VTK output
if use_vtk:
    other_includepaths.append(chaste_libs_path+'Vtk5/include/vtk-5.2')
    other_libpaths.append(chaste_libs_path+'Vtk5/lib/vtk-5.2')
    other_libraries.extend(['vtkFiltering','vtkIO',  'vtkCommon', 'vtksys', 'vtkzlib'])

# Chaste may also optionally link against CVODE.
use_cvode = False
if use_cvode:
    other_includepaths.append(chaste_libs_path+'cvode/include')
    other_libpaths.append(chaste_libs_path+'cvode/lib')
    other_libraries.extend(['sundials_cvode', 'sundials_nvecserial'])
