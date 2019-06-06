#include "BaseDemo.h"
#include <slmath/slmath.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <iomanip>

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale> 
#include <codecvt>


#include "Parser/tinyxml.h"



slmath::vec4 BaseDemo::s_Colors[] = {   slmath::vec4(1.0f, 0.1f, 0.1f, 0.45f),
                                        slmath::vec4(0.1f, 1.0f, 0.1f, 0.45f),
                                        slmath::vec4(0.1f, 0.1f, 1.0f, 0.45f),
                                        slmath::vec4(1.0f, 1.0f, 0.1f, 0.45f),
                                        slmath::vec4(1.0f, 0.1f, 1.0f, 0.45f),
                                        slmath::vec4(0.1f, 1.0f, 1.0f, 0.45f)
                                    };

const int BaseDemo::s_ColorCount = 6;

BaseDemo::BaseDemo() : m_LightPosition(0.0f)
{
}

void BaseDemo::Simulate(float /*delatT = 1 / 60.0f*/)
{
    m_PhysicsParticle.Simulate();
}

void BaseDemo::IputKey(unsigned int /*wParam*/)
{
}

void BaseDemo::Release()
{
    m_PhysicsParticle.Release();
}

slmath::vec4 *BaseDemo::GetParticlePositions() const
{
    return reinterpret_cast<slmath::vec4 *>(m_PhysicsParticle.GetParticlePositions());
}

int BaseDemo::GetParticlesCount() const
{
    return m_PhysicsParticle.GetParticlesCount();
}

bool BaseDemo::IsUsingInteroperability() const
{
    return false;
}

const slmath::vec3& BaseDemo::GetLightPosition() const
{
    return m_LightPosition;
}

void BaseDemo::InitializeGraphicsObjectToRender(DX11Renderer &renderer)
{
    renderer.InitializeParticles(GetParticlePositions(), GetParticlesCount(), 0, 0);
}

// By default all physics particles but 0 mesh
void BaseDemo::InitializeRenderer(DX11Renderer &renderer)
{
    const bool isUsingInteroperability = IsUsingInteroperability();
    renderer.SetIsUsingInteroperability(isUsingInteroperability);
    m_PhysicsParticle.SetIsUsingInteroperability(isUsingInteroperability);

    InitializeGraphicsObjectToRender(renderer);

    InitializeOpenCL(renderer);
}

void BaseDemo::InitializeOpenCL(DX11Renderer &renderer)
{
    m_PhysicsParticle.InitializeOpenCL(renderer.GetID3D11Device(), renderer.GetID3D11Buffer());
}

void BaseDemo::InitializeOpenClData()
{
    m_PhysicsParticle.InitializeOpenClData();
}

void BaseDemo::Draw(DX11Renderer &renderer) const
{
    renderer.Render(GetParticlePositions(), GetParticlesCount());
}

float BaseDemo::Compress(const slmath::vec4 &vector)
{
    // Supported compression
    assert(vector.x >= 0.0f && vector.x <= 1.0f);
    assert(vector.y >= 0.0f && vector.y <= 1.0f);
    assert(vector.z >= 0.0f && vector.z <= 1.0f);
    // Size support from 0.0f to 51.0f
    assert(vector.w >= 0.0f && vector.w <= 51.0f);

    slmath::vec4 toCompress = vector;
    unsigned int  v1 = unsigned int (toCompress.x * 0xFF);
    unsigned int  v2 = unsigned int(toCompress.y * 0xFF);
    unsigned int  v3 = unsigned int(toCompress.z * 0xFF);
    unsigned int  v4 = unsigned int (toCompress.w * 5.0f);
    
    v1 =  v1 > 255 ? 255 : v1;
    v2 =  v2 > 255 ? 255 : v2;
    v3 =  v3 > 255 ? 255 : v3;
    v4 =  v4 > 255 ? 255 : v4;
   

    unsigned int store = ((v1 & 0xFF) << 0) | ((v2 & 0xFF) << 8) | ((v3 & 0xFF) << 16) | ((v4 & 0xFF) << 24);
    float returnValue = *reinterpret_cast<float*>(&store);

    #ifdef DEBUG
        slmath::vec4 original = UnCompress(returnValue);
        const slmath::vec4 epsiolon(1.0f);
        assert(original <= toCompress + epsiolon && original >= toCompress - epsiolon );
    #endif // DEBUG

    return returnValue;
}

slmath::vec4 BaseDemo::UnCompress(float value)
{
    unsigned int uncompress = *reinterpret_cast<unsigned int*>(&value);

    unsigned int o1, o2, o3, o4;
    o1 = (unsigned int) uncompress & 0xFF;
    o2 = (unsigned int)(uncompress >> 8) & 0xFF;
    o3 = (unsigned int)(uncompress >> 16) & 0xFF;
    o4 = (unsigned int)(uncompress >> 24) & 0xFF;

    slmath::vec4 vector(float(o1) / 255.0f,
                        float(o2) / 255.0f,
                        float(o3) / 255.0f,
                        float(o4) / 5.0f);

    return vector;
}

int BaseDemo::FillPositionFromXml(const std::string& fileName, const std::string& tag, slmath::vec4* positions, int positionsCount)
{

    const std::string assetFolder ="Asset/";
    const std::string xmlExtension =".xml";
    std::string fullPath = assetFolder + fileName + xmlExtension;
    TiXmlDocument doc(fullPath.c_str());
	doc.LoadFile();

    int index = 0;
    float depth = 0.0f;
    positions[1] = slmath::vec4(0.0f);

    TiXmlElement* currentPositionElement = doc.FirstChildElement( tag.c_str() );
    TiXmlElement* firstPositionElement = currentPositionElement;
    if ( currentPositionElement )
    {
        while (currentPositionElement != NULL && index < positionsCount)
        {
            TiXmlAttribute* attribute = currentPositionElement->FirstAttribute();

            float x = 0, y = 0, z = 0;
            while (attribute != NULL)
            {
                std::stringstream stream(attribute->Value());

                if (stream.fail())
                {
                    continue;
                }
                
                if (strcmp(  attribute->Name(), "x") == 0)
                {
                    stream >> x;
                }
                if (strcmp(  attribute->Name(), "y") == 0)
                {
                    stream >> y;
                }
                if (strcmp(  attribute->Name(), "z") == 0)
                {
                    stream >> z;
                }
                  
                attribute = attribute->Next();
            }
            positions[index] = slmath::vec4(x, z + 200.0f, y + depth);

            positions[index] -= positions[0];
            positions[index] *= 0.2f;
            currentPositionElement = currentPositionElement->NextSiblingElement();

            if (currentPositionElement == NULL)
            {
                currentPositionElement = firstPositionElement;
                depth += 2.0f;
            }
            index++;
        }
    }
    return index;
}


void BaseDemo::ConvertFiles()
{

    
/* Convert 
*****************
    <trackLapPoints>
    <Sl_x0020_No>2050</Sl_x0020_No>
    <Latitude>45.51007</Latitude>
    <Longitude>-73.525185</Longitude>
    <Altitude>10</Altitude>
    <Speed>35.25</Speed>
    <Heart_x0020_Rate>0</Heart_x0020_Rate>
    <Interval_x0020_Time>2</Interval_x0020_Time>
    <Index>59213038</Index>
    <Cadence>0</Cadence>
    <PwrCadence>0</PwrCadence>
    <Power>0</Power>
  </trackLapPoints>


in ********************

<trkpt lat="45.348693" lon="-73.191907" ><ele>44</ele><speed>12.07</speed><time>2015-04-25T13:33:45Z</time>
 </trkpt>


*/


HANDLE hFind;
WIN32_FIND_DATA data;

hFind = FindFirstFile(L"E:\\Programmation\\ParticlesEngine\\Output\\Data\\*", &data);
if (hFind != INVALID_HANDLE_VALUE) {
  do {

    const std::string folder ="Data/";
    const std::string xmlExtension =".xml";
    std::wstring ws(data.cFileName);
    

    if (ws.size() < 4)
    {
        continue;
    }
    std::wstring wFileName(ws.begin(), ws.end() - 4);

	using convert_type = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_type, wchar_t> converter;

	//use converter (.to_bytes: wstr->str, .from_bytes: str->wstr)
	std::string fileName = converter.to_bytes(wFileName);

    std::string fullPath = folder + fileName + xmlExtension;

    TiXmlDocument doc(fullPath.c_str());
	doc.LoadFile();


    TiXmlElement* header = doc.FirstChildElement("GH-505_Dataform");
    // TiXmlElement* firstPositionElement = currentPositionElement;

    std::string date, startTime, time;
    float distance = 0.0f;
    std::vector<std::string> latitudes, longitudes, altitudes, speeds;
    std::vector<float> intervals;
    
    if ( header )
    {
        // Time
        TiXmlElement* currentPositionElement = header->FirstChildElement("trackHeader");
        if ( currentPositionElement )
        {
            if (currentPositionElement != NULL)
            {
                {
                    TiXmlElement* dateElement = currentPositionElement->FirstChildElement("TrackName");
                    std::stringstream stream(dateElement->GetText());
                    stream >> date;
                }
                {
                    TiXmlElement* startTimeElement = currentPositionElement->FirstChildElement("StartTime");
                    std::stringstream stream(startTimeElement->GetText());
                    stream >> startTime;
                }
                {
                    TiXmlElement* startTimeElement = currentPositionElement->FirstChildElement("TotalDist");
                    std::stringstream stream(startTimeElement->GetText());
                    stream >> distance;
                }
                {
                    TiXmlElement* startTimeElement = currentPositionElement->FirstChildElement("During");
                    std::stringstream stream(startTimeElement->GetText());
                    stream >> time;
                }
            }
        }

    // Position
        {
            TiXmlElement* currentPositionElement = header->FirstChildElement("trackLapPoints");
            if ( currentPositionElement )
            {
                while (currentPositionElement != NULL)
                {
                    std::string latitude, longitude, altitude, speed, intervalString;
                    float interval = 0;
                    {
                        TiXmlElement* element = currentPositionElement->FirstChildElement("Latitude");
                        std::stringstream stream(element->GetText());
                        stream >> latitude;
                    }
                    {
                        TiXmlElement* element = currentPositionElement->FirstChildElement("Longitude");
                        std::stringstream stream(element->GetText());
                        stream >> longitude;
                    }
                    {
                        TiXmlElement* element = currentPositionElement->FirstChildElement("Altitude");
                        std::stringstream stream(element->GetText());
                        stream >> altitude;
                    }
                    {
                        TiXmlElement* element = currentPositionElement->FirstChildElement("Speed");
                        std::stringstream stream(element->GetText());
                        stream >> speed;
                    }
                    {
                        TiXmlElement* element = currentPositionElement->FirstChildElement("Interval_x0020_Time");
                        std::stringstream stream(element->GetText());
                        stream >> intervalString;
                    }

                    latitudes.push_back(latitude);
                    longitudes.push_back(longitude);
                    altitudes.push_back(altitude);
                    speeds.push_back(speed);
                    size_t pos = intervalString.find_first_of(',');
                    if (pos != std::string::npos)
                    {
                        intervalString.replace(pos, 1,".");
                    }

                    std::istringstream convert(intervalString);
                    convert >> interval; 
                    intervals.push_back(interval);

                    
                    currentPositionElement = currentPositionElement->NextSiblingElement();
                }
            }
        }

        int hours = 0, minutes = 0, seconds = 0;
        {
            size_t hoursPosition = startTime.find_first_of(":");
            std::string hoursString = startTime.substr(0, hoursPosition);

            std::string minutesSecondsString = startTime.substr(hoursPosition + 1);

            size_t minutesPosition = minutesSecondsString.find_first_of(":");
            std::string minutesString = minutesSecondsString.substr(0, minutesPosition);
            std::string secondsString = minutesSecondsString.substr(minutesPosition + 1);

        
            std::istringstream convertH(hoursString);
            convertH >> hours; 
            std::istringstream convertM(minutesString);
            convertM >> minutes; 
            std::istringstream convertS(secondsString);
            convertS >> seconds; 
        }

        hours += 4;

        int year = 0, month = 0, day = 0;
        {
            size_t hoursPosition = date.find_first_of("-");
            std::string hoursString = date.substr(0, hoursPosition);

            std::string minutesSecondsString = date.substr(hoursPosition + 1);

            size_t minutesPosition = minutesSecondsString.find_first_of("-");
            std::string minutesString = minutesSecondsString.substr(0, minutesPosition);
            std::string secondsString = minutesSecondsString.substr(minutesPosition + 1);

        
            std::istringstream convertH(hoursString);
            convertH >> year; 
            std::istringstream convertM(minutesString);
            convertM >> month; 
            std::istringstream convertS(secondsString);
            convertS >> day; 
        }

        day -= 1;
        month += 1;

        std::ofstream gpxFile;
        gpxFile.open ("GPX/" + fileName + ".gpx");
        gpxFile << "<?xml version=\"1.0\"?>\n";
        gpxFile << "<gpx version=\"1.1\" creator=\"GlobalSat GS-Sport PC Software\" xmlns:st=\"urn:uuid:D0EB2ED5-49B6-44e3-B13C-CF15BE7DD7DD\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.topografix.com/GPX/1/1\" xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd\" >\n";
        gpxFile << "<extensions>\n";
        gpxFile << "<st:activity id=\"1\" created=\"2015-5-22,23:17:05\" startTime=\"";
        gpxFile << year << "-";
        if (month < 10)
        {
            gpxFile << "0";
        }
        gpxFile << month << "-";
        if (day < 10)
        {
            gpxFile << "0";
        }
        gpxFile << day << "T";
        if (hours < 10)
        {
            gpxFile << "0";
        }
        gpxFile << hours << ":";
        if (minutes < 10)
        {
            gpxFile << "0";
        }
        gpxFile << minutes << ":";
        if (seconds < 10)
        {
            gpxFile << "0";
        }
        gpxFile << seconds;

        gpxFile << "Z\" hasStartTime=\"true\" distanceEntered=\""<< int(distance / 1000.0) << "," << int((distance / 1000.0 - int(distance / 1000.0)) * 1000) << "\" timeEntered=\""<< time <<"\" calories=\"0\">\n";
        gpxFile << "</st:activity>\n";
        gpxFile << "</extensions>\n";
        gpxFile << "<trk>\n";
        gpxFile << "<Name>" << date << "," << startTime << "</Name>\n";
        gpxFile << "<number>1</number>\n";
        gpxFile << "<trkseg>\n";

        float intervalCumul = 0.0;
        intervals[0] = 2.0; // Hack

        for (size_t i = 0; i < latitudes.size(); ++i)
        {
            if (intervalCumul >= 1.0f)
            {
                seconds += int(intervalCumul);
                intervalCumul = (intervalCumul - int(intervalCumul));
            }
            if (seconds >= 60)
            {
                minutes++;
                seconds = seconds % 60;
            }
            if (minutes >= 60)
            {
                hours++;
                minutes = minutes % 60;
            }
            if (hours >= 24)
            {
                day++;
                hours = hours % 24;
            }

            std::cout << std::setprecision(8);
            gpxFile << "<trkpt lat=\""<< std::setprecision(8) << latitudes[i] << "\" lon=\"" << std::setprecision(8) << longitudes[i] <<"\" ><ele>" << std::setprecision(3) << altitudes[i] << "</ele><speed>" << speeds[i] <<"</speed><time>";
            gpxFile << year << "-";
            if (month < 10)
            {
                gpxFile << "0";
            }
            gpxFile << month << "-";
            if (day < 10)
            {
                gpxFile << "0";
            }
            gpxFile << day << "T";
            if (hours < 10)
            {
                gpxFile << "0";
            }
            gpxFile << hours << ":";
            if (minutes < 10)
            {
                gpxFile << "0";
            }
            gpxFile << minutes << ":";
            if (seconds < 10)
            {
                gpxFile << "0";
            }
            gpxFile << seconds << "Z</time>\n";
            gpxFile << "</trkpt>\n";

            intervalCumul += (intervals[i] - int(intervals[i]));
            int elapsedSeconds = int(intervals[i]);
            seconds += elapsedSeconds;
        }

        gpxFile << "</trkseg>\n";
        gpxFile << "</trk>\n";
        gpxFile << "</gpx>\n";

        gpxFile.close();
    }

  } while (FindNextFile(hFind, &data));
  FindClose(hFind);
}
// exit(0);

}