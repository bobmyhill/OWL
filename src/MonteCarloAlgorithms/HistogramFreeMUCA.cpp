#include <cstdio>
#include <cmath>
#include "HistogramFreeMUCA.hpp"
#include "Utilities/RandomNumberGenerator.hpp"


// Constructor
DiscreteHistogramFreeMUCA::DiscreteHistogramFreeMUCA(PhysicalSystem* ps) : h(simInfo.restartFlag, simInfo.MCInputFile, simInfo.HistogramCheckpointFile)
{

  printf("Simulation method: Histogram-free multicanonical sampling for discrete energy models\n");
  
  physical_system = ps;

  numberOfDataPoints = h.numberOfUpdatesPerIteration;
  DataSet.assign(numberOfDataPoints, 0);

}


DiscreteHistogramFreeMUCA::~DiscreteHistogramFreeMUCA()
{
 
  DataSet.clear();
  printf("Exiting DiscreteHistogramFreeMUCA class... \n");

}


void DiscreteHistogramFreeMUCA::run()
{

  //double currentTime, lastBackUpTime;
  currentTime = lastBackUpTime = MPI_Wtime();
  if (GlobalComm.thisMPIrank == 0)
    printf("Running DiscreteHistogramFreeMUCA...\n");

  char fileName[51];

  //-------------- Initialization starts --------------//

  // Find the first energy that falls within the energy range    
  while (!acceptMove) {
    physical_system -> doMCMove();
    physical_system -> getObservables();
    physical_system -> acceptMCMove();    // always accept the move to push the state forward
    acceptMove = h.checkEnergyInRange(physical_system -> observables[0]);
  }

  // Always count the first energy if it is within range
  h.updateHistogram(physical_system -> observables[0]);

  // Write out the initial configuration
  if (GlobalComm.thisMPIrank == 0)
    physical_system -> writeConfiguration(0, "initial_configuration.dat");

  //-------------- Initialization ends ---------------//

  // MUCA procedure starts here

  for (int yw=0; yw<10; yw++) {
  //while (!(h.histogramFlat)) {

    for (unsigned int MCSteps=0; MCSteps<h.numberOfUpdatesPerIteration; MCSteps++) {

      physical_system -> doMCMove();
      physical_system -> getObservables();

      // check if the energy falls within the energy range
      if ( !h.checkEnergyInRange(physical_system -> observables[0]) )
        acceptMove = false;
      else {
        // determine WL acceptance
        if ( exp(h.getDOS(physical_system -> oldObservables[0]) - 
                 h.getDOS(physical_system -> observables[0])) > getRandomNumber2() )
          acceptMove = true;
        else
          acceptMove = false;
      }

      if (acceptMove) {
         // Update histogram with trial state
         h.updateHistogram(physical_system -> observables[0]);
         h.acceptedMoves++;        

         physical_system -> acceptMCMove();
      }
      else {
         physical_system -> rejectMCMove();

         // Update histogram with old state
         h.updateHistogram(physical_system -> oldObservables[0]);
         h.rejectedMoves++;
      }
      h.totalMCsteps++;
   
      // Write restart files at interval
      currentTime = MPI_Wtime();
      if (currentTime - lastBackUpTime > checkPointInterval) {
        if (GlobalComm.thisMPIrank == 0) {
          h.writeHistogramDOSFile("hist_dos_checkpoint.dat");
          physical_system -> writeConfiguration(1, "OWL_restart_configuration");
          lastBackUpTime = currentTime;
        }
      }
    }

    // Update DOS with the histogram
    // [TODO] update scheme should be different from the original!!
    h.updateDOSwithHistogram();
    // check deviation from ideal sampling using Kullback-Leibler divergence
    h.histogramFlat = h.checkKullbackLeiblerDivergence();
      
    if (GlobalComm.thisMPIrank == 0) {
      printf("Number of iterations performed = %d\n", h.iterations);
    
      // Also write restart file here 
      sprintf(fileName, "hist_dos_iteration%02d.dat", h.iterations);
      h.writeHistogramDOSFile(fileName);
      physical_system -> writeConfiguration(1, "OWL_restart_input");
    }

    // Go to next iteration
    h.resetHistogram();
    h.iterations++;
    std::cout << "Iteration " << yw << " finished.\n";
  }

  // Write out data at the end of the simulation
  h.writeNormDOSFile("dos.dat");
  h.writeHistogramDOSFile("hist_dos_final.dat");

}



// Private member functions
void DiscreteHistogramFreeMUCA::resetDataSet()
{
  DataSet.assign(numberOfDataPoints, 0);  // To DO: check other functions!
}


