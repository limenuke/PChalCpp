/*
Author: Mark Shen
Written for Plethora's backend interview challenge in C++
Authors comments: 
I wish I did this in python. I haven't done an app to deal with web calls and data parsing
in C++ for a while and now I remember why.
*/

#include <curl/curl.h>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <cstdint>
#include <math.h>
#include <fstream>
#include <stdlib.h>
#include "rapidjson/document.h"
#include "rapidjson/rapidjson.h"

#define DEBUG 0

struct feature {    
    uint64_t theTime;
    float magnitude;
    float longitude;
    float latitude;
    float depth;
};

using namespace std;
using namespace rapidjson;

// cURL data parse callback
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Checks if item is within bounds
bool isInBounds (double value, double low, double high) {
    return !(value < low) && !(value > high);
}

//  Returns earliest timestamp in the list of captured events
uint64_t earliestEvent (std::vector<feature> capturedEvents) { 
    if (capturedEvents.size() > 1) {
        uint64_t earliestTime = capturedEvents[0].theTime;
        for (int i = 1; i < capturedEvents.size(); i++) {
            if (capturedEvents[i].theTime < earliestTime)
                earliestTime = capturedEvents[i].theTime;
        }
        return earliestTime;
    } else if (capturedEvents.size() == 0) {
        return capturedEvents[0].theTime;
    } else
        return (std::chrono::duration_cast< std::chrono::milliseconds >(std::chrono::system_clock::now().time_since_epoch())).count();
}

// Function retrieves all events that happened AFTER n days in the past but before
// the earliest event captured from the recently parsed JSON file within the n days
vector<feature> checkEvents (uint64_t myTime, std::vector<feature> capturedEvents) {
    uint64_t captureBoundary = earliestEvent(capturedEvents);

    if (DEBUG) printf("Checking events from local file quakestore.hist...between %llu and %llu\n", captureBoundary, myTime);
    vector<feature> eventList;
    FILE * pFile;
    ifstream fs;
    int lineCount = 0;
    string line;
    fs.open("quakestore.hist");
    if (fs.is_open()) {
        while (getline (fs,line)) {
            lineCount++;
            feature parseFeat;
            // Only attempts to retrieve items on every fifth line, starting with time
            // Items of interest are between the earliest data point of the freshly parsed data and
            // the n days backwards boundary. ie, On July 7th, you request the last 35 days of data.
            // The JSON request gives you 30 days so the earliest parsed entry might be only June 7th.
            // A search must be made to find events between June 2nd (35 days back) and June 7th. 
            if (lineCount == 1 && stoul(line.c_str()) >= myTime && stoul(line.c_str()) <= captureBoundary) {
                parseFeat.theTime = stoul(line.c_str());
                if (DEBUG) printf("The time is %llu\n", parseFeat.theTime);
                if (getline(fs,line)) {
                    lineCount++;
                    parseFeat.magnitude = strtof(line.c_str(), NULL);
                    if (DEBUG) printf("The magnitude is %f\n", parseFeat.magnitude);
                } else {
                    return eventList;
                }
                if (getline(fs,line)) {
                    lineCount++;
                    parseFeat.longitude = strtof(line.c_str(), NULL);
                    if (DEBUG) printf("The longitude is %f\n", parseFeat.longitude);
                } else {
                    return eventList;
                }
                if (getline(fs,line)) {
                    lineCount++;
                    parseFeat.latitude = strtof(line.c_str(),NULL);
                    if (DEBUG) printf("The latitude is %f\n", parseFeat.latitude);
                } else {
                    return eventList;
                }
                if (getline(fs,line)) {
                    lineCount++;
                    parseFeat.depth = strtof(line.c_str(),NULL);
                    if (DEBUG) printf("The depth is %f\n", parseFeat.depth);
                } else {
                    return eventList;
                }
                eventList.push_back(parseFeat);
            } else if (lineCount < 6) {
                // If the parsing algorithm gets lost through some failed getlines or we skipped an entry
                // we increment it until lineCount mod 5 == 0;
                if (DEBUG) printf("didnt match the line..\n");
                while (lineCount < 5) {
                    lineCount++;
                    if (!getline(fs,line)) {
                        return eventList;
                    }
                }
            }
            lineCount = lineCount % 5;
        }
        fs.close();
    }
    return eventList;
}

// Prints the regions
// Should be re-written to give custom region definitions instead of iterating
// through all the regions
void displayRegions (std::vector<vector<feature> > allFeats) {
    cout << endl << "REGION\t\t\tEARTHQUAKE COUNT\t\tTOTAL MAGNITUDE" << endl; 
    for (int i = 0; i < allFeats.size(); i++) {
        float magSum = 0;
        for (int j = 0; j < allFeats[i].size(); j++) {
            if (magSum == 0)
                magSum = log10(pow(10,allFeats[i][j].magnitude));
            else 
                magSum = log10(pow(10,allFeats[i][j].magnitude) + pow(10, magSum));
        }
        cout << "R" << i << "\t\t\t" <<  allFeats[i].size() << "\t\t\t\t" << magSum << endl;
    }
}

// Saves all events in current JSON capture that occur after the latest entry in the quake
// history file.
void saveEvents (vector<feature> saveList) {
    FILE * pFile;
    ifstream fs;
    int lineCount = 0;
    uint64_t myTime = 0;
    string line;
    fs.open("quakestore.hist");
    // Get latest time saved.
    if (fs.is_open()) {
        while (getline (fs,line)) {
            lineCount++;
            if (lineCount == 1) {
                if (stoull(line.c_str()) > myTime)
                    myTime = stoull(line.c_str(), NULL, 0);
            }
            lineCount = lineCount % 5;
        }
        fs.close();
    }
    ofstream fs2;
    fs2.open("quakestore.hist", std::ofstream::out | std::ofstream::app);
    if (fs2.is_open()) {
        for (int i = (saveList.size() - 1); i >= 0; i--) {  // Reverse writing starts with the earlist time
            //cout << "saveList[i].theTime is " << saveList[i].theTime << " myTime is " << myTime << endl;
            if (saveList[i].theTime > myTime) {         // Only write if the earthquake is newer than latest time
                fs2 << saveList[i].theTime << endl;
                fs2 << saveList[i].magnitude << endl;
                fs2 << saveList[i].longitude << endl;
                fs2 << saveList[i].latitude << endl;
                fs2 << saveList[i].depth << endl;
            }
        }
        fs2.close();
    }
}

// Divides the list of all features into regions based on longitude and latitude
// This can be written differently and have parameters to change the region definition
std::vector<vector<feature>> dividedRegions( std::vector<feature> allFeats) {

    std::vector<vector<feature>> allLists;
    for (int i = 0; i < 8; i++) {
        vector<feature> newList;
        allLists.push_back(newList);
    }

    for (int i = 0; i < allFeats.size(); i++) {
        feature newFeat = allFeats[i];
        if (isInBounds(newFeat.longitude, -151, -60)) { 
            if (newFeat.latitude > 0)
                allLists[0].push_back(newFeat);
            else
                allLists[1].push_back(newFeat);
        } else if (isInBounds(newFeat.longitude, -61, 30)) { 
            if (newFeat.latitude > 0)
                allLists[2].push_back(newFeat);
            else
                allLists[3].push_back(newFeat);
        } else if (isInBounds(newFeat.longitude, 29, 120)) {
            if (newFeat.latitude > 0)
                allLists[4].push_back(newFeat);
            else
                allLists[5].push_back(newFeat);
        } else if (isInBounds(newFeat.longitude, 119, 181) ||isInBounds(newFeat.longitude, -181, 150) ) {
            if (newFeat.latitude > 0)
                allLists[6].push_back(newFeat);
            else
                allLists[7].push_back(newFeat);

        }
    }
    return allLists;
}

// Retrieves all features from a full list of features within a certain time window from the
// current time as defined by unsigned int days.
std::vector<feature> currentFeatures (std::vector<feature> allFeats, unsigned int days) {
    std::vector<feature> currentList;
    std::chrono::time_point<std::chrono::system_clock> p_now = std::chrono::system_clock::now();
    uint64_t daysInMs = days * 86400000; // milliseconds in a day
    for (int i = 0; i < allFeats.size(); i++) {
        std::chrono::time_point<std::chrono::system_clock> p1;
        p1 += std::chrono::milliseconds(allFeats[i].theTime);
        auto difference = std::chrono::duration_cast<std::chrono::milliseconds>(p_now - p1).count();
        if (difference <= daysInMs) {
            currentList.push_back(allFeats[i]);
        }
    }
    if (DEBUG) cout << "Total size was " << allFeats.size() << " current list size is " << currentList.size() << std::endl;
    return currentList;
}

//  Returns all features from JSON file parsed
std::vector<feature> parseFeatures(rapidjson::Document* doc, int & size, int days) {

    vector<feature> saveList;

    Value& features = (*doc)["features"];
    assert(features.IsArray());
    using namespace std::chrono;
    int count = 0;
    for (SizeType i = 0; i < features.Size(); i++) {

        if (!features[i]["properties"].IsObject() || !features[i]["geometry"].IsObject()) {
            continue;
        }

        feature newFeat;
        Value& pC = features[i]["properties"];
        Value& geo = features[i]["geometry"];
        if (!geo["coordinates"].IsArray()) {    
            continue;
        }
        Value& coord = geo["coordinates"];
        
        if (pC["mag"].IsNumber()) {
            newFeat.magnitude = pC["mag"].GetDouble();
        } else {
            continue;            
        } 

        if (pC["time"].IsNumber()) {
            newFeat.theTime = pC["time"].GetUint64();
        } else {
            continue;            
        }

        if (coord[0].IsDouble() && coord[1].IsDouble() && coord[2].IsDouble()) {
            newFeat.longitude = coord[0].GetDouble();
            newFeat.latitude = coord[1].GetDouble();
            newFeat.depth = coord[2].GetDouble();
        } else {
            continue;            
        }
        saveList.push_back(newFeat);
        count++;
    }
    return saveList;
}

int main(int argc, char** argv)
{
    unsigned int days = 30;
    for (int i = 1; i < argc; ++i) {
        if (string(argv[i]) == "--days") {
            if (i + 1 < argc) {         // Make sure we aren't at the end of argv!
                days = atoi(argv[++i]); // Increment 'i' so we don't get the argument as the next argv[i].
            } else { // Uh-oh, there was no argument to the destination option.
                cerr << "--days option requires one argument." << endl;
                return 1;
            }  
        } else if (string(argv[i]) == "--help") {
            cerr << "Usage: " << argv[0] << endl;
            cerr << "--days <n>    n is the number of days in past to track earthquakes" << endl;
            return 1;
        } else {
            cerr << "Invalid argument detected. " << argv[i] << " is not a valid argument." << endl;
            cerr << "Usage: " << argv[0] << endl;
            cerr << "--days <n>    n is the number of days in past to track earthquakes" << endl;
            return 1;
        }
    }

    // Use cURL to grab all earthquake JSON data.
    CURL *curl;
    CURLcode res;
    string readBuffer;
    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "http://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/all_month.geojson");// "http://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/4.5_day.geojson"); //"http://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/4.5_day.geojson"); //"http://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/all_month.geojson");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    
    // Check if the JSON file has the properties we want.
    rapidjson::Document * document;
    document = new rapidjson::Document;
    using namespace rapidjson;
    document->Parse(readBuffer.c_str());
    assert(document->IsObject());
    assert(document->HasMember("features"));
    int size = 0;

    // Parse the json file into a vector of all earthquakes
    std::vector<feature>allFeats = parseFeatures(document, size, days);

    // Save all earthquakes to quake history file.
    saveEvents(allFeats);

    // Filter all earthquakes to all earthquakes that happened in the last n days.
    std::vector<feature>currFeats = currentFeatures(allFeats, days);

    // Initialize timestamp variables to retrieve earthquakes in the past through
    // quake history file
    uint64_t totalBack = 1000*60*60*24*days;    // Number of days in milliseconds to backtrack.
    uint64_t ms = (std::chrono::duration_cast< std::chrono::milliseconds >(std::chrono::system_clock::now().time_since_epoch())).count();
    vector<feature> history = checkEvents(ms-totalBack, currFeats);
    if (DEBUG) printf("The feature list size is %lu\n", history.size());
    
    // Add any retrieved historical earthquakes to the filtered list of recently parsed earthquakes
    if (history.size() > 0) {
        currFeats.insert(currFeats.end(), history.begin(), history.end());
    }

    // Filter all eathquakes by regions
    std::vector<vector<feature>> allList = dividedRegions(currFeats);

    // Print all earthquakes
    displayRegions(allList);

    return 0;
}