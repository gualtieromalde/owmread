# owmread
Read local weather data from OpenWeatherMap and feed them to FlightGear

How to use
==========

* Register on [OpenWeatherMap](https://openweathermap.org/price) and subscribe to a Free plan
* Retrieve your OpenWeatherMap API key [here](https://home.openweathermap.org/api_keys)
* Run FlightGear using the option  `--httpd=*port*`, and disable the real weather with `--disable-real-weather-fetch`.
* Run `owmread` specifying the FlightGear httpd port and your OpenWeatherMap API key (after first usage, it will be saved in a file and automatically retrieved)
* In FlightGear, select "Live data" in the Weather dialog.

How to compile
==============

`make`. Dependencies: [cURL](https://curl.haxx.se/), [Boost](https://www.boost.org/). [jsoncpp](https://github.com/open-source-parsers/jsoncpp) is included here.
