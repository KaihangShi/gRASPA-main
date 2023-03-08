#include <stdio.h>
#include <vector>
#include <string>

struct Complex
{
  double real;
  double imag;
};

struct Units
{
  double MassUnit;
  double TimeUnit;
  double LengthUnit;
  double energy_to_kelvin;
  double BoltzmannConstant;
};

struct Move_Statistics
{
  size_t NumberOfBlocks; //Number of blocks for statistical averaging
  //Translation Move//
  double TranslationProb;
  double RotationProb;
  double WidomProb;
  double SwapProb;
  double ReinsertionProb;
  //Translation Move//
  int TranslationAccepted;
  int TranslationTotal;
  double TranslationAccRatio;
  //Rotation Move//
  int RotationAccepted;
  int RotationTotal;
  double RotationAccRatio;
  //Insertion Move//
  size_t InsertionTotal;
  size_t InsertionAccepted;
  //Deletion Move//
  size_t DeletionTotal;
  size_t DeletionAccepted;
  //Reinsertion Move//
  size_t ReinsertionTotal;
  size_t ReinsertionAccepted;
};

struct Components
{
  size_t  Total_Components;                           // Number of Components in the system (including framework)
  size_t  TotalNumberOfMolecules;                     // Total Number of Molecules (including framework)
  size_t  NumberOfFrameworks;                         // Total Number of framework species, usually 1.
  double  Beta;                                       // Inverse Temperature
  std::vector<std::string> MoleculeName;              // Name of the molecule
  std::vector<size_t> Moleculesize;                   // Number of Pseudo-Atoms in the molecule
  std::vector<size_t> NumberOfMolecule_for_Component; // Number of Molecules for each component
  std::vector<size_t> Allocate_size;                  // Allocate size 
  std::vector<double> MolFraction;                    // Mol fraction of the component(excluding the framework)
  std::vector<double> IdealRosenbluthWeight;          // Ideal Rosenbluth weight
  std::vector<double> FugacityCoeff;                  // Fugacity Coefficient
  std::vector<Move_Statistics>Moves;                  // Move statistics: total, acceptance, etc.
  std::vector<double>Tc;                              // Critical Temperature of the component
  std::vector<double>Pc;                              // Critical Pressure of the component
  std::vector<double>Accentric;                       // Accentric Factor of the component
};

struct Atoms
{
  double* x;
  double* y;
  double* z;
  double* scale;
  double* charge;
  double* scaleCoul;
  size_t* Type;
  size_t* MolID;
  size_t  Molsize;
  size_t  size;
  size_t  Allocate_size;
};

struct Simulations //For multiple simulations//
{
  Atoms*  d_a;                  // Pointer For Atom Data in the Simulation Box //
  Atoms   Old;                  // Temporary data storage for Old Configuration //
  Atoms   New;                  // Temporary data storage for New Configuration //
  Atoms*  deviceOld; 
  Atoms*  deviceNew;
  double* Blocksum;             // Block sums for partial reduction //
  bool*   flag;                 // flags for checking overlaps //
  bool*   device_flag;          // flags for overlaps on the device //
  double3 MaxTranslation;
  double3 MaxRotation;          
};

struct PseudoAtomDefinitions //Always a host struct, never on the device
{
  std::vector<std::string> Name;
  std::vector<double> oxidation;
  std::vector<double> mass;
  std::vector<double> charge;
  std::vector<double> polar; //polarizability
};

struct ForceField
{
  double* epsilon;
  double* sigma;
  double* z; // a third term
  double* shift;
  int*    FFType; //type of force field calculation  
  bool    noCharges;
  size_t  size;
  double  OverlapCriteria;
  double  CutOffVDW;
  double  CutOffCoul;
  double  Prefactor;
  double  Alpha;
};

struct Boxsize
{
  //Complex* eik_x;
  //Complex* eik_y;
  //Complex* eik_z;
  //Complex* eik_xy;
  //Complex* storedEik;
  float*   float_Cell;
  float*   float_InverseCell;
  double*  Cell;
  double*  InverseCell;
  double   Pressure;
  double   Volume;
  double   Temperature;
  bool     Cubic;
  //size_t   alloc_x_size;
  //size_t   alloc_y_size;
  //size_t   alloc_z_size;
  //size_t   alloc_xy_size;
  //size_t   alloc_wavevector;
};

struct WidomStruct
{
  bool                UseGPUReduction;              // For calculating the energies for each bead
  bool                Useflag;                      // For using flags (for skipping reduction)
  size_t              NumberWidomTrials;            // Number of Trial Positions for the first bead //
  size_t              NumberWidomTrialsOrientations;// Number of Trial Orientations 
  size_t              WidomFirstBeadAllocatesize;   //space allocated for WidomFirstBeadResult
  std::vector<double> Rosenbluth;                   
  std::vector<double> RosenbluthSquared;
  std::vector<double> ExcessMu;
  std::vector<double> ExcessMuSquared;
  std::vector<int>    RosenbluthCount;
};

struct RandomNumber
{
  double* host_random;
  double* device_random;
  size_t  randomsize;
  size_t  offset;
};
