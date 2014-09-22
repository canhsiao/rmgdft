
#include <exception>
#include <iostream>
#include <vector>
#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/lexical_cast.hpp>

#include "make_conf.h"
#include "const.h"
#include "rmgtypedefs.h"
#include "params.h"
#include "typedefs.h"
#include "rmg_error.h"


// Converts a string containg a PP_MESH, PP_RAB, PP_LOCAL, etc into a double array and
// returns a pointer to the array. The calling function needs to have obtained the 
// number of elements expected in the array from the attributes field.
double * UPF_str_to_double_array(std::string str, int max_count) {

   std::vector<std::string> strs;
   int count = 0;
   double *array = new double[max_count];
   std::string delims = " \t\n";
   boost::trim(str);
   boost::algorithm::split( strs, str, boost::is_any_of(delims), boost::token_compress_on );
 
   std::vector<std::string>::iterator it;
   for (it = strs.begin(); it != strs.end(); ++it) {
       std::string svalue = *it;
       boost::trim(svalue);
       array[count] = std::atof(svalue.c_str());
       count++;
       if(count > max_count)
           rmg_error_handler(__FILE__,__LINE__,"Problem with UPF pseudopotential. Too many elements.\n");
   }

   return array;
}


using boost::property_tree::ptree;

// Reads a pseudopotential stored in UPF format into our internal data structures.
void LoadUpf(char *file, SPECIES *sp)
{

   ptree upf_tree;
   std::string PP_INFO;
   std::string upf_file = file;
 
   read_xml(upf_file, upf_tree);
   PP_INFO = upf_tree.get<std::string>("UPF.PP_INFO"); 
   
   std::cout << PP_INFO << std::endl; 

   // Get the type of pseudopotential
   std::string pp_type = upf_tree.get<std::string>("UPF.PP_HEADER.<xmlattr>.pseudo_type");
   boost::to_upper(pp_type);
   if(!pp_type.compare(0, 2, "NC")) {
       ct.norm_conserving_pp = true;
   }
   else if(!pp_type.compare(0, 2, "US")) {
       ct.norm_conserving_pp = false;
   }
   else {
       rmg_error_handler(__FILE__,__LINE__,"RMG only supports norm conserving and ultrasoft pseudpotentials.\n");
   }


   // Kind of redundant information in the format
   std::string s_is_ultrasoft = upf_tree.get<std::string>("UPF.PP_HEADER.<xmlattr>.is_ultrasoft"); 
   boost::to_upper(s_is_ultrasoft);
   if(!s_is_ultrasoft.compare(0,1,"F")) ct.norm_conserving_pp = true;
   if(!s_is_ultrasoft.compare(0,5,"FALSE")) ct.norm_conserving_pp = true;
   if(!s_is_ultrasoft.compare(0,1,"T")) ct.norm_conserving_pp = false;
   if(!s_is_ultrasoft.compare(0,4,"TRUE")) ct.norm_conserving_pp = false;

   // Core correction
   std::string s_core_correction = upf_tree.get<std::string>("UPF.PP_HEADER.<xmlattr>.core_correction");
   if(!s_core_correction.compare(0,1,"F")) sp->nlccflag = false;
   if(!s_core_correction.compare(0,5,"FALSE")) sp->nlccflag = false;
   if(!s_core_correction.compare(0,1,"T")) sp->nlccflag = true;
   if(!s_core_correction.compare(0,4,"TRUE")) sp->nlccflag = true;

   // Attributes of the mesh
   double PP_MESH_dx = upf_tree.get<double>("UPF.PP_MESH.<xmlattr>.dx");
   std::cout << "PP_MESH.dx    =  " << PP_MESH_dx << std::endl; 

   sp->rg_points = upf_tree.get<double>("UPF.PP_MESH.<xmlattr>.mesh");
   std::cout << "PP_MESH.mesh  =  " << sp->rg_points << std::endl; 
 
   double PP_MESH_xmin = upf_tree.get<double>("UPF.PP_MESH.<xmlattr>.xmin");
   std::cout << "PP_MESH.xmin  =  " << PP_MESH_xmin << std::endl; 

   double PP_MESH_rmax = upf_tree.get<double>("UPF.PP_MESH.<xmlattr>.rmax");
   std::cout << "PP_MESH.rmax  =  " << PP_MESH_rmax << std::endl; 

   double PP_MESH_zmesh = upf_tree.get<double>("UPF.PP_MESH.<xmlattr>.zmesh");
   std::cout << "PP_MESH.zmesh  =  " << PP_MESH_zmesh << std::endl; 


   // Read in the radial mesh and convert it into a C style array
   std::string PP_R = upf_tree.get<std::string>("UPF.PP_MESH.PP_R");
   sp->r = UPF_str_to_double_array(PP_R, sp->rg_points);

   // Read in rab and convert it into a C style array
   std::string PP_RAB = upf_tree.get<std::string>("UPF.PP_MESH.PP_RAB");
   sp->rab = UPF_str_to_double_array(PP_RAB, sp->rg_points);

   // Local potential
   std::string PP_LOCAL = upf_tree.get<std::string>("UPF.PP_LOCAL");
   sp->vloc0 = UPF_str_to_double_array(PP_LOCAL, sp->rg_points);

   // Atomic charge density
   std::string PP_RHOATOM = upf_tree.get<std::string>("UPF.PP_RHOATOM");
   sp->atomic_rho = UPF_str_to_double_array(PP_RHOATOM, sp->rg_points);

   // Number of atomic orbitals
   int number_of_wfc = upf_tree.get<double>("UPF.PP_HEADER.<xmlattr>.number_of_wfc");
   if(number_of_wfc > 0) {

       for(int iwf = 0;iwf < number_of_wfc;iwf++) {
           // Ugh. UPF format has embedded .s so use / as a separator
           typedef ptree::path_type path;
           std::string chi = "UPF/PP_PSWFC/PP_CHI." + boost::lexical_cast<std::string>(iwf + 1);
           std::string PP_CHI = upf_tree.get<std::string>(path(chi, '/'));
           sp->atomic_wave[iwf] = UPF_str_to_double_array(PP_CHI, sp->rg_points);
           
       }

   }

   // Number of projectors
   sp->nbeta = upf_tree.get<double>("UPF.PP_HEADER.<xmlattr>.number_of_proj");
   if(sp->nbeta > 0) {

       for(int ip = 0;ip < sp->nbeta;ip++) {
           // Ugh. UPF format has embedded .s so use / as a separator
           typedef ptree::path_type path;
           std::string betapath = "UPF/PP_NONLOCAL/PP_BETA." + boost::lexical_cast<std::string>(ip + 1);
           std::string PP_BETA = upf_tree.get<std::string>(path(betapath, '/'));
           sp->beta[ip] = UPF_str_to_double_array(PP_BETA, sp->rg_points);
            
       }

   }


   
}

// C binding
extern "C" void LoadUpf_C(char *file, SPECIES *sp)
{
    LoadUpf_C(file, sp);
}

