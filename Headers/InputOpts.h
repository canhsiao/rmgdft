#ifndef RMG_InputOpts_H
#define RMG_InputOpts_H 1

#include <unordered_map>

static std::unordered_map<std::string, int> bravais_lattice_type = {
        {"None", 0},
        {"Cubic Primitive", 1},
        {"Cubic Face Centered", 2},
        {"Cubic Body Centered", 3},
        {"Hexagonal Primitive", 4},
        {"Hexagonal Rhombohedral (Trigonal)", 5},
        {"Tetragonal Primitive", 6},
        {"Tetragonal Body Centered", 7},
        {"Orthorhombic Primitive", 8},
        {"Orthorhombic Base Centered", 9},
        {"Orthorhombic Body Centered", 10},
        {"Orthorhombic Face Centered", 11},
        {"Monoclinic Primitive", 12},
        {"Monoclinic Base Centered", 13},
        {"Triclinic Primitive", 14}};

static std::unordered_map<std::string, int> discretization_type = {
        {"Mehrstellen", 0},
        {"Central", 1}};

static std::unordered_map<std::string, int> calculation_mode = {
        {"Quench Electrons", 0},
        {"Relax Structure", 1},
        {"Constant Volume And Energy", 2},
        {"Constant Temperature And Energy", 3},
        {"Constant Pressure And Energy", 4},
        {"Plot", 5},
        {"Psi Plot", 6},
        {"Band Structure Only", 7},
        {"NEB Relax", 8},
        {"Dimer Relax", 9}};

static std::unordered_map<std::string, int> occupations_type = {
        {"Fermi Dirac", 0},
        {"Gaussian", 1},
        {"Error Function", 2}};

static std::unordered_map<std::string, int> exchange_correlation_type = {
        {"GGA BLYP", 0},
        {"GGA XB CP", 1},
        {"GGA XP CP", 2},
        {"GGA PBE", 3},
        {"MGGA TB09", 4}};


#endif