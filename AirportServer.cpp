#include <iostream>
#include <thread>
#include <condition_variable>

#include "AirportServer.h"


/**
*  Called by an Airplane when it wishes to land on a runway
*/
AirportServer::AirportServer()
{
	// ***** Initialize any Locks and/or Condition Variables here as necessary *****
	sectR = false;
	sect15R = false;
	sect15L = false;
	sect4R = false;
	sect4L = false;
	sect14 = false;
}
void AirportServer::reserveRunway(int airplaneNum, AirportRunways::RunwayNumber runway)
{
	// Acquire runway(s)
	{  // Begin critical region

		//unique_lock<mutex> runwaysLock(runwaysMutex);

		{
			unique_lock<mutex> runwaysLock(runwaysMutex);
			lock_guard<mutex> lk(AirportRunways::checkMutex);

			cout << "Airplane #" << airplaneNum << " is acquiring any needed runway(s) for landing on Runway "
				 << AirportRunways::runwayName(runway) << endl;
		}

		/**
		 *  ***** Add your synchronization here! *****
		 */
		if(AirportRunways::runwayName(runway) == "9")
			{
			//sectR = true;
			unique_lock<mutex> ulkR(secR);
			while(sectR || sect4R || sect15R) section_R.wait(ulkR);
			sectR = true;	
			}
		else if(AirportRunways::runwayName(runway) == "15R")
			{
			//sectR = true;
			//sect15 = true;
			unique_lock<mutex> ulk15R(sec15R);
			while(sectR || sect4R || sect15R || sect4L)section_15R.wait(ulk15R);
			sect15R = true;	
			}
		
		else if(AirportRunways::runwayName(runway) == "15L")
			{
			//sect15 = true;	
			unique_lock<mutex> ulk15L(sec15L);
			while(sect4R || sect4L || sect15L)section_15L.wait(ulk15L);
			sect15L = true;	
			}
		else if(AirportRunways::runwayName(runway) == "4R")
			{
			//sectR = true;
			//sect4 = true;
			unique_lock<mutex> ulk4R(sec4R);
			while(sect4R || sect15R || sect15L || sectR)section_4R.wait(ulk4R);
			sect4R = true;	
			}
		
		else if(AirportRunways::runwayName(runway) == "4L")
			{
			//sect4L = true;
			unique_lock<mutex> ulk4L(sec4L);
			while(sect15R || sect15L || sect4L)section_4L.wait(ulk4L);
			sect4L = true;	
			}
		else if(AirportRunways::runwayName(runway) == "14")
			{
			//sect14 = true;
			unique_lock<mutex> ulk14(sec14);
			while(sect14)section_14.wait(ulk14);
			sect14 = true;	
			}
		unique_lock<mutex> runwaysLock(runwaysMutex);
		// Check status of the airport for any rule violations
		AirportRunways::checkAirportStatus(runway);

		runwaysLock.unlock();

	} // End critical region

	// obtain a seed from the system clock:
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator(seed);

	// Taxi for a random number of milliseconds
	std::uniform_int_distribution<int> taxiTimeDistribution(1, MAX_TAXI_TIME);
	int taxiTime = taxiTimeDistribution(generator);

	{
		lock_guard<mutex> lk(AirportRunways::checkMutex);

		cout << "Airplane #" << airplaneNum << " is taxiing on Runway " << AirportRunways::runwayName(runway)
			 << " for " << taxiTime << " milliseconds\n";
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(taxiTime));

} // end AirportServer::reserveRunway()


 /**
  *  Called by an Airplane when it is finished landing
  */
void AirportServer::releaseRunway(int airplaneNum, AirportRunways::RunwayNumber runway)
{
	// Release the landing runway and any other needed runways
	{ // Begin critical region


		{
			unique_lock<mutex> runwaysLock(runwaysMutex);
			lock_guard<mutex> lk(AirportRunways::checkMutex);

			cout << "Airplane #" << airplaneNum << " is releasing any needed runway(s) after landing on Runway "
				 << AirportRunways::runwayName(runway) << endl;
		}

		/**
		*  ***** Add your synchronization here! *****
		*/
		if(AirportRunways::runwayName(runway) == "9")
			{
			//sectR = true;
			unique_lock<mutex> ulkR(secR);
			section_R.notify_one();
			sectR = false;	
			}
		else if(AirportRunways::runwayName(runway) == "15R")
			{
			//sectR = true;
			//sect15 = true;
			unique_lock<mutex> ulk15R(sec15R);
			section_15R.notify_one();
			sect15R = false;	
			}
		
		else if(AirportRunways::runwayName(runway) == "15L")
			{
			//sect15 = true;	
			unique_lock<mutex> ulk15L(sec15L);
			section_15L.notify_one();
			sect15L = false;	
			}
		else if(AirportRunways::runwayName(runway) == "4R")
			{
			//sectR = true;
			//sect4 = true;
			unique_lock<mutex> ulk4R(sec4R);
			section_4R.notify_one();
			sect4R = false;	
			}
		
		else if(AirportRunways::runwayName(runway) == "4L")
			{
			//sect4L = true;
			unique_lock<mutex> ulk4L(sec4L);
			section_4L.notify_one();
			sect4L = false;	
			}
		else if(AirportRunways::runwayName(runway) == "14")
			{
			//sect14 = true;
			unique_lock<mutex> ulk14(sec14);
			section_14.notify_one();
			sect14 = false;	
			}
		// Update the status of the airport to indicate that the landing is complete
		unique_lock<mutex> runwaysLock(runwaysMutex);
		
		AirportRunways::finishedWithRunway(runway);

		runwaysLock.unlock();

	} // End critical region

	// obtain a seed from the system clock:
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator(seed);

	// Wait for a random number of milliseconds before requesting the next landing for this Airplane
	std::uniform_int_distribution<int> waitTimeDistribution(1, MAX_WAIT_TIME);
	int waitTime = waitTimeDistribution(generator);

	{
		lock_guard<mutex> lk(AirportRunways::checkMutex);

		cout << "Airplane #" << airplaneNum << " is waiting for " << waitTime << " milliseconds before landing again\n";
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));

} // end AirportServer::releaseRunway()
