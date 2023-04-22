#include <complex>
#include <vector>
#define M_PI           3.14159265358979323846

inline void Ewald_PBC_CPU(double* posvec, double* Cell, double* InverseCell, bool Cubic)
{
  if(Cubic)//cubic/cuboid
  {
    posvec[0] = posvec[0] - static_cast<int>(posvec[0] * InverseCell[0*3+0] + ((posvec[0] >= 0.0) ? 0.5 : -0.5)) * Cell[0*3+0];
    posvec[1] = posvec[1] - static_cast<int>(posvec[1] * InverseCell[1*3+1] + ((posvec[1] >= 0.0) ? 0.5 : -0.5)) * Cell[1*3+1];
    posvec[2] = posvec[2] - static_cast<int>(posvec[2] * InverseCell[2*3+2] + ((posvec[2] >= 0.0) ? 0.5 : -0.5)) * Cell[2*3+2];
  }
  else
  {
    double s[3] = {0.0, 0.0, 0.0};
    s[0]=InverseCell[0*3+0]*posvec[0]+InverseCell[1*3+0]*posvec[1]+InverseCell[2*3+0]*posvec[2];
    s[1]=InverseCell[0*3+1]*posvec[0]+InverseCell[1*3+1]*posvec[1]+InverseCell[2*3+1]*posvec[2];
    s[2]=InverseCell[0*3+2]*posvec[0]+InverseCell[1*3+2]*posvec[1]+InverseCell[2*3+2]*posvec[2];

    s[0] -= static_cast<int>(s[0] + ((s[0] >= 0.0) ? 0.5 : -0.5));
    s[1] -= static_cast<int>(s[1] + ((s[1] >= 0.0) ? 0.5 : -0.5));
    s[2] -= static_cast<int>(s[2] + ((s[2] >= 0.0) ? 0.5 : -0.5));
    // convert from abc to xyz
    posvec[0]=Cell[0*3+0]*s[0]+Cell[1*3+0]*s[1]+Cell[2*3+0]*s[2];
    posvec[1]=Cell[0*3+1]*s[0]+Cell[1*3+1]*s[1]+Cell[2*3+1]*s[2];
    posvec[2]=Cell[0*3+2]*s[0]+Cell[1*3+2]*s[1]+Cell[2*3+2]*s[2];
  }
}

/*
__device__ void Ewald_PBC(double* posvec, double* Cell, double* InverseCell, bool Cubic)
{
  if(Cubic)//cubic/cuboid
  {
    posvec[0] = posvec[0] - static_cast<int>(posvec[0] * InverseCell[0*3+0] + ((posvec[0] >= 0.0) ? 0.5 : -0.5)) * Cell[0*3+0];
    posvec[1] = posvec[1] - static_cast<int>(posvec[1] * InverseCell[1*3+1] + ((posvec[1] >= 0.0) ? 0.5 : -0.5)) * Cell[1*3+1];
    posvec[2] = posvec[2] - static_cast<int>(posvec[2] * InverseCell[2*3+2] + ((posvec[2] >= 0.0) ? 0.5 : -0.5)) * Cell[2*3+2];
  }
  else
  {
    double s[3] = {0.0, 0.0, 0.0};
    s[0]=InverseCell[0*3+0]*posvec[0]+InverseCell[1*3+0]*posvec[1]+InverseCell[2*3+0]*posvec[2];
    s[1]=InverseCell[0*3+1]*posvec[0]+InverseCell[1*3+1]*posvec[1]+InverseCell[2*3+1]*posvec[2];
    s[2]=InverseCell[0*3+2]*posvec[0]+InverseCell[1*3+2]*posvec[1]+InverseCell[2*3+2]*posvec[2];

    s[0] -= static_cast<int>(s[0] + ((s[0] >= 0.0) ? 0.5 : -0.5));
    s[1] -= static_cast<int>(s[1] + ((s[1] >= 0.0) ? 0.5 : -0.5));
    s[2] -= static_cast<int>(s[2] + ((s[2] >= 0.0) ? 0.5 : -0.5));
    // convert from abc to xyz
    posvec[0]=Cell[0*3+0]*s[0]+Cell[1*3+0]*s[1]+Cell[2*3+0]*s[2];
    posvec[1]=Cell[0*3+1]*s[0]+Cell[1*3+1]*s[1]+Cell[2*3+1]*s[2];
    posvec[2]=Cell[0*3+2]*s[0]+Cell[1*3+2]*s[1]+Cell[2*3+2]*s[2];
  }
}

*/
void matrix_multiply_by_vector(double* a, double* b, double* c) //3x3(9*1) matrix (a) times 3x1(3*1) vector (b), a*b=c//
{
  c[0]=a[0*3+0]*b[0]+a[1*3+0]*b[1]+a[2*3+0]*b[2];
  c[1]=a[0*3+1]*b[0]+a[1*3+1]*b[1]+a[2*3+1]*b[2];
  c[2]=a[0*3+2]*b[0]+a[1*3+2]*b[1]+a[2*3+2]*b[2];
}

double Ewald_Total(Boxsize& Box, Atoms*& Host_System, ForceField& FF, Components& SystemComponents)
{
  double recip_cutoff = Box.ReciprocalCutOff;
  int kx_max = Box.kmax.x;
  int ky_max = Box.kmax.y;
  int kz_max = Box.kmax.z;
  if(FF.noCharges) return 0.0;
  double alpha = Box.Alpha; double alpha_squared = alpha * alpha;
  double prefactor = Box.Prefactor * (2.0 * M_PI / Box.Volume);

 
  double ewaldE = 0.0;
 
  double ax[3] = {Box.InverseCell[0], Box.InverseCell[3], Box.InverseCell[6]}; //printf("ax: %.10f, %.10f, %.10f\n", Box.InverseCell[0], Box.InverseCell[3], Box.InverseCell[6]);
  double ay[3] = {Box.InverseCell[1], Box.InverseCell[4], Box.InverseCell[7]}; //printf("ay: %.10f, %.10f, %.10f\n", Box.InverseCell[1], Box.InverseCell[4], Box.InverseCell[7]);
  double az[3] = {Box.InverseCell[2], Box.InverseCell[5], Box.InverseCell[8]}; //printf("az: %.10f, %.10f, %.10f\n", Box.InverseCell[2], Box.InverseCell[5], Box.InverseCell[8]);
  
  size_t numberOfAtoms = 0;
  for(size_t i=0; i < SystemComponents.Total_Components; i++) //Skip the first one(framework)
  {
    numberOfAtoms  += SystemComponents.Moleculesize[i] * SystemComponents.NumberOfMolecule_for_Component[i];
  }
  //size_t numberOfWaveVectors = (kx_max + 1) * (2 * ky_max + 1) * (2 * kz_max + 1);
  //Zhao's note: if starting with an empty box, numberOfAtoms = 0, but to allocate space on the GPU, you cannot do zero space for an array//
  //Here, we use 2 * adsorbate_size, since this is the max size gonna be used in the Monte Carlo steps//
  size_t eik_atomsize = *std::max_element(std::begin(SystemComponents.Moleculesize), std::end(SystemComponents.Moleculesize));
  eik_atomsize *= 2;
  if(eik_atomsize < numberOfAtoms) eik_atomsize = numberOfAtoms;
  std::vector<std::complex<double>>eik_x(eik_atomsize * (kx_max + 1));
  std::vector<std::complex<double>>eik_y(eik_atomsize * (ky_max + 1));
  std::vector<std::complex<double>>eik_z(eik_atomsize * (kz_max + 1));
  std::vector<std::complex<double>>eik_xy(eik_atomsize);
  size_t numberOfWaveVectors = (Box.kmax.x + 1) * (2 * Box.kmax.y + 1) * (2 * Box.kmax.z + 1);
  std::vector<std::complex<double>>storedEik(numberOfWaveVectors);
  std::vector<std::complex<double>>FrameworkEik(numberOfWaveVectors);
  // Construct exp(ik.r) for atoms and k-vectors kx, ky, kz = 0, 1 explicitly
  size_t count=0;
  for(size_t comp=0; comp < SystemComponents.Total_Components; comp++)
  {
    for(size_t posi=0; posi < SystemComponents.NumberOfMolecule_for_Component[comp] * SystemComponents.Moleculesize[comp]; posi++)
    {
      //determine the component for i
      double pos[3] = {Host_System[comp].x[posi], Host_System[comp].y[posi], Host_System[comp].z[posi]};
      eik_x[count + 0 * numberOfAtoms] = std::complex<double>(1.0, 0.0);
      eik_y[count + 0 * numberOfAtoms] = std::complex<double>(1.0, 0.0);
      eik_z[count + 0 * numberOfAtoms] = std::complex<double>(1.0, 0.0);
      double s[3]; matrix_multiply_by_vector(Box.InverseCell, pos, s); for(size_t j = 0; j < 3; j++) s[j]*=2*M_PI;
      eik_x[count + 1 * numberOfAtoms] = std::complex<double>(std::cos(s[0]), std::sin(s[0]));
      eik_y[count + 1 * numberOfAtoms] = std::complex<double>(std::cos(s[1]), std::sin(s[1]));
      eik_z[count + 1 * numberOfAtoms] = std::complex<double>(std::cos(s[2]), std::sin(s[2]));
      count++;
    }
  }
  // Calculate remaining positive kx, ky and kz by recurrence
  for(size_t kx = 2; kx <= kx_max; ++kx)
  {
    for(size_t i = 0; i != numberOfAtoms; ++i)
    {
      eik_x[i + kx * numberOfAtoms] = eik_x[i + (kx - 1) * numberOfAtoms] * eik_x[i + 1 * numberOfAtoms];
    }
  }
  for(size_t ky = 2; ky <= ky_max; ++ky)
  {
    for(size_t i = 0; i != numberOfAtoms; ++i)
    {
      eik_y[i + ky * numberOfAtoms] = eik_y[i + (ky - 1) * numberOfAtoms] * eik_y[i + 1 * numberOfAtoms];
    }
  }
  for(size_t kz = 2; kz <= kz_max; ++kz)
  {
    for(size_t i = 0; i != numberOfAtoms; ++i)
    {
      eik_z[i + kz * numberOfAtoms] = eik_z[i + (kz - 1) * numberOfAtoms] * eik_z[i + 1 * numberOfAtoms];
    }
  }
  size_t nvec = 0;
  //for debugging
  double FrameworkEwald = 0.0;
  size_t kxcount = 0; size_t kycount = 0; size_t kzcount = 0; size_t kzinactive = 0;
  for(std::make_signed_t<std::size_t> kx = 0; kx <= kx_max; ++kx)
  {
    double kvec_x[3]; for(size_t j = 0; j < 3; j++) kvec_x[j] = 2.0 * M_PI * static_cast<double>(kx) * ax[j];
    // Only positive kx are used, the negative kx are taken into account by the factor of two
    double factor = (kx == 0) ? (1.0 * prefactor) : (2.0 * prefactor);
    
    for(std::make_signed_t<std::size_t> ky = -ky_max; ky <= ky_max; ++ky)
    {
      double kvec_y[3]; for(size_t j = 0; j < 3; j++) kvec_y[j] = 2.0 * M_PI * static_cast<double>(ky) * ay[j];
      // Precompute and store eik_x * eik_y outside the kz-loop
      for(size_t i = 0; i != numberOfAtoms; ++i)
      {
        std::complex<double> eiky_temp = eik_y[i + numberOfAtoms * static_cast<size_t>(std::abs(ky))];
        eiky_temp.imag(ky>=0 ? eiky_temp.imag() : -eiky_temp.imag());
        eik_xy[i] = eik_x[i + numberOfAtoms * static_cast<size_t>(kx)] * eiky_temp;
      }

      for(std::make_signed_t<std::size_t> kz = -kz_max; kz <= kz_max; ++kz)
      {
        // Ommit kvec==0
        size_t ksqr = kx * kx + ky * ky + kz * kz;
        std::complex<double> cksum(0.0, 0.0);
        std::complex<double> Frameworkck(0.0, 0.0);
        if((ksqr != 0) && (static_cast<double>(ksqr) < recip_cutoff))
        {
          double kvec_z[3]; for(size_t j = 0; j < 3; j++) kvec_z[j] = 2.0 * M_PI * static_cast<double>(kz) * az[j];
          //std::complex<double> cksum(0.0, 0.0);
          count=0;
          for(size_t comp=0; comp<SystemComponents.Total_Components; comp++)
          {
            for(size_t posi=0; posi<SystemComponents.NumberOfMolecule_for_Component[comp]*SystemComponents.Moleculesize[comp]; posi++)
            {
              std::complex<double> eikz_temp = eik_z[count + numberOfAtoms * static_cast<size_t>(std::abs(kz))];
              eikz_temp.imag(kz>=0 ? eikz_temp.imag() : -eikz_temp.imag());
              double charge = Host_System[comp].charge[posi];
              double scaling = Host_System[comp].scaleCoul[posi];
              cksum += scaling * charge * (eik_xy[count] * eikz_temp);
              if(comp == 0 && SystemComponents.NumberOfFrameworks > 0) Frameworkck += scaling * charge * (eik_xy[count] * eikz_temp);
              count++;
            }
          }
          //double rksq = (kvec_x + kvec_y + kvec_z).length_squared();
          double tempkvec[3] = {kvec_x[0]+kvec_y[0]+kvec_z[0], kvec_x[1]+kvec_y[1]+kvec_z[1], kvec_x[2]+kvec_y[2]+kvec_z[2]};
          double rksq = tempkvec[0]*tempkvec[0] + tempkvec[1]*tempkvec[1] + tempkvec[2]*tempkvec[2];
          double temp = factor * std::exp((-0.25 / alpha_squared) * rksq) / rksq;
          if(SystemComponents.NumberOfFrameworks > 0 && Box.ExcludeHostGuestEwald)
            cksum -= Frameworkck;
          double tempsum = temp * (cksum.real() * cksum.real() + cksum.imag() * cksum.imag());
          double tempFramework = temp * (Frameworkck.real() * Frameworkck.real() + Frameworkck.imag() * Frameworkck.imag());
          ewaldE += tempsum;
          FrameworkEwald += tempFramework;
          //storedEik[nvec] = cksum;
          //++nvec;
          kzcount++;
        }
        FrameworkEik[nvec] = Frameworkck;
        storedEik[nvec]    = cksum;
        /*
        if(Box.ExcludeHostGuestEwald) 
        {
          storedEik[nvec] -= FrameworkEik[nvec]; //exclude Framework contribution in Eik, this will also exclude it on the GPU and kernel functions
        }
        */
        ++nvec;
        kzinactive++;
      }
      kycount++;
    }
    kxcount++;
  }

  printf("Total ewaldE Fourier: %.10f, Framework Fourier: %.10f\n", ewaldE, FrameworkEwald);
  if(Box.ExcludeHostGuestEwald) ewaldE += FrameworkEwald;
  

  // Subtract self-energy
  double prefactor_self = Box.Prefactor * alpha / std::sqrt(M_PI);
  count=0;
  for(size_t comp=0; comp<SystemComponents.Total_Components; comp++)
  {
    double SelfE = 0.0;
    for(size_t posi=0; posi<SystemComponents.NumberOfMolecule_for_Component[comp]*SystemComponents.Moleculesize[comp]; posi++)
    {
      double charge = Host_System[comp].charge[posi];
      double scaling = Host_System[comp].scaleCoul[posi];
      ewaldE -= prefactor_self * scaling * charge * scaling * charge;
      SelfE  += prefactor_self * scaling * charge * scaling * charge;
      if(comp == 0 && SystemComponents.NumberOfFrameworks > 0) FrameworkEwald -= prefactor_self * scaling * charge * scaling * charge;
    }
    printf("Component: %zu, SelfAtomE: %.5f\n", comp, SelfE);
  }
  //printf("After Self ewaldE: %.10f\n", ewaldE);


  // Subtract exclusion-energy, Zhao's note: taking out the pairs of energies that belong to the same molecule
  size_t j_count = 0;
  for(size_t l = 0; l != SystemComponents.Total_Components; ++l)
  {
    double exclusionE = 0.0;  
    //printf("Exclusion on component %zu, size: %zu\n", l, Host_System[l].size);
    if(Host_System[l].size != 0)
    {
      for(size_t mol = 0; mol != SystemComponents.NumberOfMolecule_for_Component[l]; mol++)
      {
        size_t AtomID = mol * SystemComponents.Moleculesize[l];
        for(size_t i = AtomID; i != AtomID + SystemComponents.Moleculesize[l] - 1; i++)
        {
          double factorA = Host_System[l].scaleCoul[i] * Host_System[l].charge[i];
          double posA[3] = {Host_System[l].x[i], Host_System[l].y[i], Host_System[l].z[i]};
          for(size_t j = i + 1; j != AtomID + SystemComponents.Moleculesize[l]; j++)
          {
            double factorB = Host_System[l].scaleCoul[j] * Host_System[l].charge[j];
            double posB[3] = {Host_System[l].x[j], Host_System[l].y[j], Host_System[l].z[j]};

            double posvec[3] = {posA[0]-posB[0], posA[1]-posB[1], posA[2]-posB[2]};
            PBC_CPU(posvec, Box.Cell, Box.InverseCell, Box.Cubic);
            double rr_dot = posvec[0]*posvec[0] + posvec[1]*posvec[1] + posvec[2]*posvec[2];
            double r = std::sqrt(rr_dot);

            ewaldE     -= Box.Prefactor * factorA * factorB * std::erf(alpha * r) / r;
            exclusionE -= Box.Prefactor * factorA * factorB * std::erf(alpha * r) / r;
            if(l == 0 && SystemComponents.NumberOfFrameworks > 0) FrameworkEwald -= Box.Prefactor * factorA * factorB * std::erf(alpha * r) / r;
            j_count++;
          }
        }
      }
    }
    printf("Component: %zu, Intra-Molecular ExclusionE: %.5f\n", l, exclusionE);
  }
  printf("Framework Ewald: %.10f\n", FrameworkEwald);
  SystemComponents.FrameworkEwald = FrameworkEwald;

  /*
  if(Box.ExcludeHostGuestEwald)
    ewaldE -= FrameworkEwald;
  */
  //Record the values for the Ewald Vectors//
//  for(size_t i = 0; i < eik_xy.size(); i++)
    SystemComponents.eik_xy       = eik_xy;
//  for(size_t i = 0; i < eik_x.size(); i++)
    SystemComponents.eik_x        = eik_x;
//  for(size_t i = 0; i < eik_y.size(); i++)
    SystemComponents.eik_y        = eik_y;
//  for(size_t i = 0; i < eik_z.size(); i++)
    SystemComponents.eik_z        = eik_z;
//  for(size_t i = 0; i < storedEik.size(); i++)
    SystemComponents.storedEik    = storedEik;

    SystemComponents.FrameworkEik = FrameworkEik;
    //IF RUN ON CPU, check totalEik vs. storedEik//
    /*
    if(SystemComponents.totalEik.size() > 0)
    {
      for(size_t i = 0; i < storedEik.size(); i++)
      {
        if((SystemComponents.storedEik[i].real() != SystemComponents.totalEik[i].real()) || (SystemComponents.storedEik[i].imag() != SystemComponents.totalEik[i].imag()))
        {
          printf("element %zu: stored: %.15f %.15f, updated: %.15f %.15f\n", i, storedEik[i].real(), storedEik[i].imag(), SystemComponents.totalEik[i].real(), SystemComponents.totalEik[i].imag());
        }
      }
    }
    */
  return ewaldE;
}

double Calculate_Intra_Molecule_Exclusion(Boxsize& Box, Atoms* System, double alpha, double Prefactor, Components& SystemComponents, size_t SelectedComponent)
{
  double E = 0.0;
  if(SystemComponents.Moleculesize[SelectedComponent] == 0) return 0.0;
  for(size_t i = 0; i != SystemComponents.Moleculesize[SelectedComponent] - 1; i++)
  {
    double factorA = System[SelectedComponent].scaleCoul[i] * System[SelectedComponent].charge[i];
    double posA[3] = {System[SelectedComponent].x[i], System[SelectedComponent].y[i], System[SelectedComponent].z[i]};
    for(size_t j = i + 1; j != SystemComponents.Moleculesize[SelectedComponent]; j++)
    {
      double factorB = System[SelectedComponent].scaleCoul[j] * System[SelectedComponent].charge[j];
      double posB[3] = {System[SelectedComponent].x[j], System[SelectedComponent].y[j], System[SelectedComponent].z[j]};

      double posvec[3] = {posA[0]-posB[0], posA[1]-posB[1], posA[2]-posB[2]};
      PBC_CPU(posvec, Box.Cell, Box.InverseCell, Box.Cubic);
      double rr_dot = posvec[0]*posvec[0] + posvec[1]*posvec[1] + posvec[2]*posvec[2];
      double r = std::sqrt(rr_dot);

      E += Prefactor * factorA * factorB * std::erf(alpha * r) / r;
    }
  }
  printf("Component %zu, Intra Exclusion Energy: %.5f\n", SelectedComponent, E);
  return E;
}

double Calculate_Self_Exclusion(Boxsize& Box, Atoms* System, double alpha, double Prefactor, Components& SystemComponents, size_t SelectedComponent)
{
  double E = 0.0; double prefactor_self = Prefactor * alpha / std::sqrt(M_PI);
  if(SystemComponents.Moleculesize[SelectedComponent] == 0) return 0.0;
  for(size_t i=0; i<SystemComponents.Moleculesize[SelectedComponent]; i++)
  {
    double charge = System[SelectedComponent].charge[i];
    double scaling = System[SelectedComponent].scaleCoul[i];
    E += prefactor_self * scaling * charge * scaling * charge;
  }
  printf("Component %zu, Atom Self Exclusion Energy: %.5f\n", SelectedComponent, E);
  return E;
}

void Check_WaveVector_CPUGPU(Boxsize& Box, Components& SystemComponents)
{
  size_t numberOfWaveVectors = (Box.kmax.x + 1) * (2 * Box.kmax.y + 1) * (2 * Box.kmax.z + 1);
  Complex GPUWV[numberOfWaveVectors];
  cudaMemcpy(GPUWV, Box.storedEik, numberOfWaveVectors * sizeof(Complex), cudaMemcpyDeviceToHost);
  size_t numWVCPU            = SystemComponents.storedEik.size();
  if(numberOfWaveVectors != numWVCPU) printf("ERROR: Number of CPU WaveVectors does NOT EQUAL to the GPU one!!!");
  size_t counter = 0;
  for(size_t i = 0; i < numberOfWaveVectors; i++)
  {
    double diff_real = abs(SystemComponents.storedEik[i].real() - GPUWV[i].real);
    double diff_imag = abs(SystemComponents.storedEik[i].imag() - GPUWV[i].imag);
    if(i < 10)
      printf("Wave Vector %zu, CPU: %.5f %.5f, GPU: %.5f %.5f\n", i, SystemComponents.storedEik[i].real(), SystemComponents.storedEik[i].imag(), GPUWV[i].real, GPUWV[i].imag);
    if(diff_real > 1e-10 || diff_imag > 1e-10)
    {
      counter++;
      if(counter < 10) printf("There is a difference in GPU/CPU WaveVector at position %zu: CPU: %.5f %.5f, GPU: %.5f %.5f\n", i, SystemComponents.storedEik[i].real(), SystemComponents.storedEik[i].imag(), GPUWV[i].real, GPUWV[i].imag);
    }
  }
  if(counter >= 10) printf("More than 10 WaveVectors mismatch.\n");
  //Also check Framework Eik vectors//
  for(size_t i = 0; i < numberOfWaveVectors; i++)
  {
    if(i < 10) printf("Framework Wave Vector %zu, real: %.5f imag: %.5f\n", i, SystemComponents.FrameworkEik[i].real(), SystemComponents.FrameworkEik[i].imag());
  }
}
