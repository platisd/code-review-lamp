# Corona lamp
Corona lamp is a twist over the [Code Review lamp](https://github.com/platisd/code-review-lamp)
to raise awareness over the spread of the [CoViD-19](https://en.wikipedia.org/wiki/Coronavirus_disease_2019).

## What?

Corona lamp illuminates according to the growth rate of the CoViD-19 infections in the selected country,
over the past 2 weeks. Overall, the more red the lamp is, the higher the infection rate. When it gets
greener, it means that the growth rate of the disease is low or has even stopped.

There are **16** LEDs on the lamp, each illuminated for the disease's growth rate on that particular
day. The data are fetched wirelessly, via a ESP8266 microcontroller and their original source currently
is Center for Systems Science and Engineering
([CSSE](https://www.arcgis.com/apps/opsdashboard/index.html#/bda7594740fd40299423467b48e9ecf6)) at
Johns Hopkins University.

## Why?

Corona lamp, can be utilized to quickly monitor the progress of the country's efforts to halt
the disease, without being as *threatening* as graphs and sometimes misleading as plain numbers.


## How?

New data are fetched via the [covid-api](https://covid-api.quintessential.gr/) by
[Quintessential SFT](https://www.quintessential.gr/) every 12 hours. The source code for the service
can be found [here](https://github.com/Quintessential-SFT/Covid-19-API/).

Then, the growth rate for each day over the last (approximately) two weeks is calcutated and
assigned to a color.

### Dependencies

Fetch the following resources via the Arduino IDE library manager:

* [ArduinoJson](https://arduinojson.org/)
* [NTPClient](https://github.com/arduino-libraries/NTPClient)
