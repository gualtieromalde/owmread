#include<memory>
#include"inc/json/json.h"
#include<curl/curl.h>
#include<iostream>
#include<fstream>
#include<ctime>
#include<string>
#include<boost/format.hpp>
#include<boost/program_options.hpp>
#include <chrono>
#include <thread>

//using namespace std;

struct Wind
{
	double speed;
	int deg;
	double gust;
};

double const ms_to_kt = 1.94384;
double const kel_to_c = 273.15;

struct Weather
{
	double temperature;
	double dewpoint;
	double relhum;
	int visibility;
	int cloudiness;
	int code;
	Wind wind;
	double pressure;
	int error;
	Weather() {}
	Weather(int a): error(a) {} // error constructor	
	std::string makeMetar();
};

struct Coord
{
	double lat;
	double lon;
	Coord(double la, double lo) : lat(la), lon(lo) {}
};

double getDewPoint(double temperature, double relhum)
{
	return temperature - .2*(100.-relhum);
}

std::string getUrl(double lat, double lon, std::string key)
{

	const std::string basicurl="http://api.openweathermap.org/data/2.5/weather?";
	const std::string sLat = "lat=";
	const std::string sLon = "&lon=";
	const std::string sappid = "&appid=";
	return basicurl+sLat+std::to_string(lat)+sLon+std::to_string(lon)+sappid + key;
}

static size_t no_write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
   return size * nmemb;
}
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

Weather getWeather(double lon, double lat, std::string myKey)
{
    
	
    Weather weather;
    const std::string url = getUrl(lon,lat,myKey);
    CURL* curl = curl_easy_init();

    // Set remote URL.
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    long httpCode(0);
    std::string httpData;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &httpData);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_cleanup(curl);

    if (httpCode != CURLE_HTTP_RETURNED_ERROR)
    {

        Json::Value jsonData;
        Json::Reader jsonReader;
	std::string err;
        if ( jsonReader.parse(httpData, jsonData)   ) 
        {

   	    weather.temperature = jsonData["main"]["temp"].asDouble() - kel_to_c;
            weather.pressure = jsonData["main"]["pressure"].asDouble();
	    /// set default pressure for absurd values
	    if(weather.pressure < 800 || weather.pressure > 1200) weather.pressure = 1013;
	    weather.relhum = jsonData["main"]["humidity"].asDouble();
	    weather.dewpoint = getDewPoint(weather.temperature, weather.relhum);

	    weather.wind.speed = jsonData["wind"]["speed"].asDouble() * ms_to_kt;
	    weather.wind.gust = jsonData["wind"]["gust"].asDouble() * ms_to_kt;
	    weather.wind.deg = jsonData["wind"]["deg"].asDouble();

	    weather.visibility = jsonData["visibility"].asInt();
	    if(weather.visibility>9999) weather.visibility=9999;

	    weather.cloudiness = jsonData["clouds"]["all"].asInt();
	    weather.code = jsonData["weather"][0]["id"].asInt();
	    //...
	    return weather;
        }
        else
        {
            std::cerr << "Could not parse HTTP data as JSON" << std::endl;
            return Weather(1);
       	} 
    }
    else
    {
        std::cerr << "Couldn't get from " << url << " - exiting" << std::endl;
        return Weather(0);
    }
}

std::string Weather::makeMetar()
{
	std::string s;
	const std::string sbase = "XXXX 012345Z ";
	const std::string space = " ";
	std::string swind; 

	boost::format windFormat("%03i%02iKT");
	boost::format windGustFormat("%03i%02iG%02iKT");
	boost::format tempFormat("%02i/%02i");
	boost::format pressFormat("Q%04i");

	if((int)wind.gust==0)
		swind = str( windFormat % (int)wind.deg % (int)wind.speed) ;
	else
		swind = str( windGustFormat % (int)wind.deg % (int)wind.speed % (int)wind.gust);

	const std::string svis = std::to_string((int)visibility);

	const std::string stemp = str( tempFormat % (int)temperature % (int)dewpoint);

	const std::string spress = str( pressFormat % (int)pressure);

	const std::string cloudstat[] = {"NCD", "FEW", "SCT", "BKN", "OVC"};
	std::string scloud;
	switch(cloudiness)
	{
	case 0 ... 10:	scloud = cloudstat[0]; break;
	case 11 ... 25:	scloud = cloudstat[1]; break;
	case 26 ... 50:	scloud = cloudstat[2]; break;
	case 51 ... 84:	scloud = cloudstat[3]; break;
	case 85 ... 100:scloud = cloudstat[4]; break;
	}

	std::string scode;
	switch(code)
	{
		case 200: scode="-TSRA"; break;
		case 201: scode="TSRA"; break;
		case 202: scode="+TSRA"; break;
		case 210: scode="-TS"; break;
		case 211: case 221: scode="TS"; break;
		case 212: scode="+TS"; break;
		case 230: scode="-TSDZ"; break;
		case 231: scode="TSDZ"; break;
		case 232: scode="+TSDZ"; break;
	
		case 300: case 310: scode="-DZ"; break;
		case 301: case 311: scode="DZ"; break;
		case 302: case 312: scode="+DZ"; break;
		case 313: case 321: scode="SHDZ"; break;
		case 314: scode="+SHDZ"; break;

		case 500: scode="-RA"; break;
		case 501: scode="RA"; break;
		case 502: case 503: case 504: scode="+RA"; break;
		case 511: scode="FZRA"; break;
		case 520: scode="-SHRA"; break;
		case 521: case 531: scode="SHRA"; break;
		case 522: scode="+SHRA"; break;		    

		case 600: case 615: scode="-SN"; break;
		case 601: case 616: scode="SN"; break;
		case 602: scode="+SN"; break;
		case 611: scode="SG"; break;
		case 612: scode="-SHSG"; break;
		case 613: scode="+SHSG"; break;
		case 620: scode="-SHSN"; break;
		case 621: scode="SHSN"; break;
		case 622: scode="+SHSN"; break;

		case 701: scode="BR"; break;
		case 711: scode="FU"; break;
		case 721: scode="HZ"; break;
		case 731: scode="PO"; break;
		case 741: scode="FG"; break;
		case 751: scode="SA"; break;
		case 761: scode="DU"; break;
		case 762: scode="VA"; break;
		case 771: scode="SQ"; break;
		case 781: scode="FC"; break;
	}

	s = sbase + swind + space + svis + space + scode + space + scloud + space + stemp + space + spress + "\n";
	return s;	
}

Coord readCoord(int port)
{
    const std::string url_lat = "http://localhost:" + std::to_string(port) + "/json/position/latitude-deg";
    const std::string url_lon = "http://localhost:" + std::to_string(port) + "/json/position/longitude-deg";

    CURL* curlLat = curl_easy_init();

    curl_easy_setopt(curlLat, CURLOPT_URL, url_lat.c_str());
    curl_easy_setopt(curlLat, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
    curl_easy_setopt(curlLat, CURLOPT_TIMEOUT, 10);
    curl_easy_setopt(curlLat, CURLOPT_FOLLOWLOCATION, 1L);
    long httpCode(0);
    std::string httpDataLat;
    curl_easy_setopt(curlLat, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curlLat, CURLOPT_WRITEDATA, &httpDataLat);
    curl_easy_perform(curlLat);
    curl_easy_getinfo(curlLat, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_cleanup(curlLat);
    Json::Value jsonDataLat;
    Json::Reader jsonReaderLat;
    jsonReaderLat.parse(httpDataLat, jsonDataLat);   
    double lat = jsonDataLat["value"].asDouble();	     
    
     CURL* curlLon = curl_easy_init();

    curl_easy_setopt(curlLon, CURLOPT_URL, url_lon.c_str());
    curl_easy_setopt(curlLon, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
    curl_easy_setopt(curlLon, CURLOPT_TIMEOUT, 10);
    curl_easy_setopt(curlLon, CURLOPT_FOLLOWLOCATION, 1L);
    long httpCodeLon(0);
    std::string httpDataLon;
    curl_easy_setopt(curlLon, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curlLon, CURLOPT_WRITEDATA, &httpDataLon);
    curl_easy_perform(curlLon);
    curl_easy_getinfo(curlLon, CURLINFO_RESPONSE_CODE, &httpCodeLon);
    curl_easy_cleanup(curlLon);
    Json::Value jsonDataLon;
    Json::Reader jsonReaderLon;
    jsonReaderLon.parse(httpDataLon, jsonDataLon);   
    double lon = jsonDataLon["value"].asDouble();	     
    
    
    
    return Coord(lat,lon);
}

void setMetar(int port, std::string metar)
{
    CURL* curl = curl_easy_init();
    const std::string url = "http://localhost:" + std::to_string(port) + "/props/environment/metar?submit=set&data[0]=" + curl_easy_escape(curl, metar.c_str(), metar.size());
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, no_write_data);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
}
int main(int nargs, char ** args)
{
	boost::program_options::options_description desc("owmread reads OpenWeatherMap current weather and feed it to FlightGear. Options:");
	desc.add_options()
    	("help", "produce help message")
    	("key", boost::program_options::value<std::string>(), "set OpenWeatherMap api key (if not specified search from file key, or from standard input)")
    	("port", boost::program_options::value<int>(), "set httpd port to communicate with FlightGear (fgfs must run with --httpd=port)")
	("interval", boost::program_options::value<int>()->default_value(10), "number of seconds between each API call. Default is 10. Note that, for the free OpenWeatherMap account the limit is set to 60 calls/minute and 1M calls/month, so a safe limit is interval>=3 s.")
    	("quiet", "does not display output")
;

	boost::program_options::variables_map vm;
	boost::program_options::store(boost::program_options::parse_command_line(nargs, args, desc), vm);
	boost::program_options::notify(vm);   

	bool ovr_key;
	bool isQuiet;

	int sleep_time =  vm["interval"].as<int>(); 
	if(sleep_time<=1)
	{
		std::clog << "Sleep time <= 1 s. This could generate a number of API calls per minute not allowed in OpenWeatherMap free plan. Continue? y/n: ";
		char yn; std::cin >> yn;
		if(yn!='y' || yn!='Y') return 2;	
	}
	else if(sleep_time<3)
	{
		std::clog << "Sleep time < 3 s. This could generate a number of API calls per month not allowed in OpenWeatherMap free plan. Continue? y/n: ";
		char yn; std::cin >> yn;
		if(yn!='y' || yn!='Y') return 2;	
	}
		


	std::string argKey;

	if(vm.count("help"))
	{
		std::clog << desc <<"\n";
		return 1;
	}

	if(vm.count("key")) 
	{
		ovr_key=true;
	      	argKey = vm["key"].as<std::string>();
	}
	else 
		ovr_key=false;

	int port;
	if(vm.count("port"))
	{
		port=vm["port"].as<int>();
	}
	else
	{
		std::cout << "insert port: ";
		std::cin >> port;
	}

	isQuiet = vm.count("quiet");

	std::string myKey;
	std::ifstream keyFile("key");
	if(!ovr_key && keyFile.good() )
	{
		keyFile >> myKey;
		keyFile.close();
	}
	else if(!ovr_key)
	{
		std::clog << "insert key: ";
		std::cin >> myKey;
		std::ofstream kF("key");
		kF << myKey;
		kF.close();
	}
	else
	{
		myKey =	argKey;
	}
	std::chrono::seconds interval( sleep_time ) ;


	while(true)
	{
		auto now = std::chrono::system_clock::now();
		std::time_t now_time = std::chrono::system_clock::to_time_t(now);
		Coord c =readCoord(port);
		Weather w = getWeather(c.lat, c.lon, myKey);
		setMetar(port, w.makeMetar());
		if(!isQuiet) std::cout << std::ctime(&now_time) <<c.lat << " " << c.lon << " " << " " << w.makeMetar();

		std::this_thread::sleep_for( interval ) ;
	}
}

