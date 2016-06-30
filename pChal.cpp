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
//{"type":"Feature","properties":{"mag":1.64,"place":"8km WNW of The Geysers, California","time":1466382598720,"updated":1466383565477,"tz":-420,"url":"http://earthquake.usgs.gov/earthquakes/eventpage/nc72653201","detail":"http://earthquake.usgs.gov/earthquakes/feed/v1.0/detail/nc72653201.geojson","felt":null,"cdi":null,"mmi":null,"alert":null,"status":"automatic","tsunami":0,"sig":41,"net":"nc","code":"72653201","ids":",nc72653201,","sources":",nc,","types":",focal-mechanism,general-link,geoserve,nearby-cities,origin,phase-data,scitech-link,","nst":30,"dmin":0.005801,"rms":0.05,"gap":80,"magType":"md","type":"earthquake","title":"M 1.6 - 8km WNW of The Geysers, California"},"geometry":{"type":"Point","coordinates":[-122.8421631,38.8214989,1.72]},"id":"nc72653201"},
//{"type":"FeatureCollection","metadata":{"generated":1466383643000,"url":"http://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/all_month.geojson","title":"USGS All Earthquakes, Past Month","status":200,"api":"1.5.2","count":9534},"features":[{"type":"Feature","properties":{"mag":0.82,"place":"8km NE of San Jacinto, CA","time":1466383383650,"updated":1466383603380,"tz":-420,"url":"http://earthquake.usgs.gov/earthquakes/eventpage/ci37392599","detail":"http://earthquake.usgs.gov/earthquakes/feed/v1.0/detail/ci37392599.geojson","felt":null,"cdi":null,"mmi":null,"alert":null,"status":"automatic","tsunami":0,"sig":10,"net":"ci","code":"37392599","ids":",ci37392599,","sources":",ci,","types":",general-link,geoserve,nearby-cities,origin,phase-data,scitech-link,","nst":25,"dmin":0.06184,"rms":0.18,"gap":80,"magType":"ml","type":"earthquake","title":"M 0.8 - 8km NE of San Jacinto, CA"},"geometry":{"type":"Point","coordinates":[-116.898

struct feature {    
    uint64_t theTime;
    float magnitude;
    float longitude;
    float latitude;
    float depth;
};

using namespace std;
using namespace rapidjson;

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

bool isInBounds (double value, double low, double high) {
    return !(value < low) && !(value > high);
}

void saveEvents (vector<feature> saveList) {
    // File format:
    // time
    // mag
    // long
    // lat
    // depth
    FILE * pFile;
    ifstream fs;
    int lineCount = 0;
    uint64_t myTime = 0;
    string line;
    fs.open("quakestore.hist");//, std::fstream::in | std::fstream::out | std::fstream::app);
    if (fs.is_open()) {
        while (getline (fs,line)) {
            lineCount++;
            if (lineCount == 1) {
                if (atoi(line.c_str()) > myTime)
                    myTime = stoul(line.c_str(), NULL, 0);
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
            if (saveList[i].theTime > myTime) {         // Only write if the earthquake is newer...
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

std::vector<vector<feature> > parseFeatures(rapidjson::Document* doc, int & size, int days) {

    vector<vector<feature> > allLists;
    for (int i = 0; i < 8; i++) {
        vector<feature> featList;
        allLists.push_back(featList);
    }
    vector<feature> saveList;

    Value& features = (*doc)["features"];
    assert(features.IsArray());
    using namespace std::chrono;
    int count = 0;
    for (SizeType i = 0; i < features.Size(); i++) {
            std::chrono::time_point<std::chrono::system_clock> p1;
            std::chrono::time_point<std::chrono::system_clock> p_now = system_clock::now();
            long unsigned int daysInMs = days * 86400000; // milliseconds in a day

            if (!features[i]["properties"].IsObject() || !features[i]["geometry"].IsObject()) {
                continue;
            }

            feature newFeat;
            Value& pC = features[i]["properties"];
            Value& geo = features[i]["geometry"];
            if (!geo["coordinates"].IsArray()) {    
                //printf("\nEARLY SKIP\n");
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
                //std::cout << newFeat.theTime << std::endl;
            } else {
                continue;            
            }

            p1 += + std::chrono::milliseconds(newFeat.theTime);
            auto difference = std::chrono::duration_cast<std::chrono::milliseconds>(p_now - p1).count();
            //cout << "The difference was " << difference << " and the tolerance is " << daysInMs << endl;
            if (difference > daysInMs) {
                continue;
            }
            // If the coordinate 
            if (coord[0].IsDouble() && coord[1].IsDouble() && coord[2].IsDouble()) {
                newFeat.longitude = coord[0].GetDouble();
                newFeat.latitude = coord[1].GetDouble();
                newFeat.depth = coord[2].GetDouble();
                //printf("[ %f    %f ]\n", newFeat.latitude, newFeat.longitude);
            } else {
                //printf("Skipped! 2\n");
                continue;            
            }
            
            //printf("Mag is %f\n", newFeat.magnitude);//featList[size].magnitude);
            //printf("Time is %f\n", newFeat.theTime); //featList[size].theTime);
                    // work with parameterB[i]["status"], parameterB[i]["B_1"], etc.
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
            saveList.push_back(newFeat);
            count++;

    }
    int total = allLists[0].size() + allLists[1].size() + allLists[2].size() + allLists[3].size() + allLists[4].size() + allLists[5].size() + allLists[6].size() + allLists[7].size();
    saveEvents(saveList);
    //printf("Region events:\n%lu\n%lu\n%lu\n%lu\n%lu\n%lu\n%lu\n%lu\n",allLists[0].size(),allLists[1].size(),allLists[2].size(),allLists[3].size(),allLists[4].size(),allLists[5].size(),allLists[6].size(),allLists[7].size());
    //printf("Total is %d and count is %d\n", total, count);    
    return allLists;
}



void displayRegions (std::vector<vector<feature> > allFeats) {
    cout << "REGION\t\t\tEARTHQUAKE COUNT\t\tTOTAL MAGNITUDE" << endl; 
    for (int i = 0; i < allFeats.size(); i++) {
        float magSum = 0;
        for (int j = 0; j < allFeats[i].size(); j++) {
            magSum += log(allFeats[i][j].magnitude);
        }
        cout << "R" << i << "\t\t\t" << 10 * allFeats[i].size() << "\t\t\t\t" << magSum << endl;
    }
}

int main(int argc, char** argv)
{
    int days = 30;
    for (int i = 1; i < argc; ++i) {
        if (string(argv[i]) == "--days") {
            if (i + 1 < argc) { // Mak  e sure we aren't at the end of argv!
                days = atoi(argv[++i]); // Increment 'i' so we don't get the argument as the next argv[i].
                printf ("new days is %d\n",days);
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

    CURL *curl;
    CURLcode res;
    string readBuffer;
    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "http://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/4.5_day.geojson"); //"http://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/4.5_day.geojson"); //"http://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/all_month.geojson");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    rapidjson::Document * document;
    document = new rapidjson::Document;
    using namespace rapidjson;
    document->Parse(readBuffer.c_str());
    assert(document->IsObject());
    assert(document->HasMember("features"));
    int size = 0;

    std::vector<vector<feature> > allFeats = parseFeatures(document, size, days);
    displayRegions(allFeats);

    return 0;
}


/*




int main (int argc, char* argv[]) {

    curl_global_init(CURL_GLOBAL_ALL);
        CURL *handle = curl_easy_init(); 
        curl_easy_setopt(handle, CURLOPT_URL, "http://localhost:9200/_search?" + query_string)
        

        if (curl) printf("curl_easy_init() succeeded!\n"); 
        else fprintf(stderr, "Error calling curl_easy_init().\n");
    return 0;
}*/