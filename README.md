# Code Review Lamp
A colorful lamp to notify the developer team for pending code reviews.
![Code Review Lamp](https://i.imgur.com/V9rwpnD.jpg)

## What?
Code Review Lamp is a Neopixel-based, WiFi-enabled gadget that reminds developers to peer-review their colleagues' code. For each submission that has not been reviewed enough, it dims up and down at a color that is specific to the developer who is trying to introduce a new functionality to a project. The lamp stops shining once the code has either received enough reviews by the team, been merged or designated as *Work In Progress*. Currently, it is configured to work with [Gerrit](https://www.gerritcodereview.com/) (v.2.15) or GitHub, but it could be programmed to fetch data from different tools such as Jenkins, GitLab etc.

The gadget is comprised of an [ESP8266 microcontroller module](https://wiki.wemos.cc/products:d1:d1_mini) that connects wirelessly to the internet, as well as a [Neopixel ring](https://www.adafruit.com/product/1463) which displays various colors.

## Why?
At work, we are a fast-paced team that likes to commit (relatively) small and often. Every commit does not only have to pass the various tests that are being run by our Continuous Integration machinery, but also gets extensively code reviewed before allowed to be merged. Since we are *six* in total and our internal code of conduct dictates *two* positive approvals for each new submission, the lack of responsiveness when it comes to reviewing, can impede the development process and speed.

In an ideal world, developers would check their notification emails and would proceed on with reviewing their peer's code. However, as they are too absorbed by their ongoing work and perhaps slightly overwhelmed by the amount of emails they receive, these emails tend to be ignored. The common thought is that someone else in the team is going to review the code, but often no one does.

We have employed two different techniques so far to solve the problem. The first one includes shouting **CODE REVIEW** loudly and the second placing color-coded bean bags (I am not kidding :laughing:) on each others' desks. The third that hopes to rule them all is the Code Review Lamp!

Its advantage is that it continuously, and discretely, reminds developers that **the team** needs to review code and thus someone's work is being blocked as long as the lamp is shining. Additionally, they can see who is requesting the code review and how many reviews need to be conducted.

## How?
Reaping the benefits of the Code Review Lamp is simple for the development team member. They merely need to push code to Gerrit, add the group which includes all team members to the review and set the review to *Work in Progress* or *Ready for review* depending on whether they want the team to start or stop reviewing the code.

We have included a special user in our team's Gerrit group, representing the lamp, which is added along the team to each review. This user is being utilized by an ESP8266 microcontroller which connects to WiFi and [queries](https://gerrit-review.googlesource.com/Documentation/rest-api-changes.html) Gerrit for open reviews where this "lamp user" is a reviewer. Of course, it is not necessary to have a separate user for the lamp. Any team member's already existing account can be used and this way Code Review Lamps can be personalized. However, since we consider code reviews a *team activity* and the foundation of cross-functionality and knowledge-sharing, we have opted for the "team approach".

Next, for each review the user is assigned to, we determine how many reviews have been conducted and if they fall short of our agreed threshold, a color that corresponds to the *owner* of the review is displayed on the Neopixel ring.

### Software
The [Gerrit firmware](https://github.com/platisd/code-review-lamp/blob/master/firmware/gerrit_watcher/gerrit_watcher.ino), compatible with ESP8266 microcontrollers, utilizes the [Adafruit Neopixel](https://github.com/adafruit/Adafruit_NeoPixel) library to control the Neopixel ring. When using **Gerrit**, the JSON response is parsed manually, as existing solutions which would nicely serialize the whole stream would frequently cause heap overflow.

The [GitHub firmware](https://github.com/platisd/code-review-lamp/blob/master/firmware/github_watcher/github_watcher.ino) on the other hand does not parse the JSON response coming from **GitHub** itself directly. This is due to the API response being overly verbose for the ESP8266 microcontroller, which renders parsing it on the microcontroller infeasible. Instead, an external server has to be used which will process the data and merely send what colors should be displayed. An example of such a server, written in Python3, can be found [here](https://github.com/platisd/code-review-lamp/blob/master/firmware/github_watcher/github_reviews_watcher.py).

### Hardware
The gadget is rather simple to source and assemble. The only "custom" part is the [PCB](https://github.com/platisd/code-review-lamp/tree/master/hardware/gerrit_lamp) which merely connects the ESP8266 module with the Neopixel ring.

### Get started
After you have assembled the hardware, flash the firmware by following the steps below:
* If on Windows or Mac, download the [Serial to USB chip drivers](https://wiki.wemos.cc/downloads)
* Download and install the latest [Arduino IDE](https://www.arduino.cc/en/Main/Software) for your distribution
* Install the `Adafruit Neopixel` library
  * In Arduino IDE, click on `Sketch` :arrow_right: `Include Library` :arrow_right: `Manage Libraries`
  * Look for `Adafruit NeoPixel` and install the `Adafruit NeoPixel by adafruit`
* Install the ESP8266 SDK
  * In Arduino IDE, click on `File` :arrow_right: `Preferences` :arrow_right: `Additional Board Manager URLs`
  * Paste `http://arduino.esp8266.com/stable/package_esp8266com_index.json` and click `OK`
  * In Arduino IDE, click on `Tools` :arrow_right: `Board` :arrow_right: `Boards Manager`
  * Look for `esp8266` and install the `esp8266 by ESP8266 Community`
* Select the Wemos D1 Mini board
  * In Arduino IDE, click on `Tools` :arrow_right: `Board` :arrow_right: `LOLIN(WEMOS) D1 R2 & mini`
* Select the serial port your Code Review Lamp is connected to
  * In Arduino IDE, click on `Tools` :arrow_right: `Port`
* Copy & paste the [code](https://github.com/platisd/code-review-lamp/blob/master/firmware/gerrit_watcher/gerrit_watcher.ino) to your IDE
* Make the necessary adjustments for your own SSID, username etc
  * GitHub Token
    * Create a [personal access token](https://github.com/settings/tokens)
    * Give it all `repo` permissions if you want it to access your private repositories too, otherwise just `public_repo`
* Upload the firmware by clicking `Upload` (the right arrow button on the upper left corner of your IDE)

### How to use
* Commit your change to Gerrit or GitHub
* Add the group which includes the team as reviewers
  * A special "notification" user that will trigger the lamp should be part of the group
* ???
* Get your code reviewed faster!

## Components
* [Code Review Lamp PCB](https://www.pcbway.com/project/shareproject/W17435BSW42_code_review_lamp.html)
* [3D printed case](https://www.tinkercad.com/things/evNud1d8GYI)
* [16 RGB LED Neopixel Ring](https://www.adafruit.com/product/1463)
* [Wemos D1 Mini](https://wiki.wemos.cc/products:d1:d1_mini)
* 330Ohm resistor
* 470uF/16V capacitor
* M3x40mm screws (4)
* M3 nuts (4)
* Micro USB cable

## Media
* Article on [platis.solutions](https://platis.solutions/blog/2018/09/26/code-review-lamp/)
* [Demo video](https://www.youtube.com/watch?v=TPO2nQkfprY)
