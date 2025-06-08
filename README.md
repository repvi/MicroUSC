<!-- Improved compatibility of back to top link: See: https://github.com/othneildrew/Best-README-Template/pull/73 -->
<a id="readme-top"></a>
<!--
*** Thanks for checking out the Best-README-Template. If you have a suggestion
*** that would make this better, please fork the repo and create a pull request
*** or simply open an issue with the tag "enhancement".
*** Don't forget to give the project a star!
*** Thanks again! Now go create something AMAZING! :D
-->

<!-- PROJECT SHIELDS -->
<!--
*** I'm using markdown "reference style" links for readability.
*** Reference links are enclosed in brackets [ ] instead of parentheses ( ).
*** See the bottom of this document for the declaration of the reference variables
*** for contributors-url, forks-url, etc. This is an optional, concise syntax you may use.
*** https://www.markdownguide.org/basic-syntax/#reference-style-links
-->
[![Contributors][contributors-shield]][contributors-url]
[![Forks][forks-shield]][forks-url]
[![Stargazers][stars-shield]][stars-url]
[![Issues][issues-shield]][issues-url]
[![project_license][license-shield]][license-url]
[![LinkedIn][linkedin-shield]][linkedin-url]

| Current Known Supported Targets | ESP32 | ESP32-S3 |
| ------------------------------- | ----- | -------- |

<!-- TABLE OF CONTENTS -->
<details>
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#about-the-project">About The Project</a>
    </li>
    <li>
      <a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#prerequisites">Prerequisites</a></li>
        <li><a href="#installation">Installation</a></li>
      </ul>
    </li>
    <li><a href="#usage">Usage</a></li>
    <!--<li><a href="#roadmap">Roadmap</a></li> -->
    <li><a href="#contributing">Contributing</a></li>
    <li><a href="#license">License</a></li>
    <!--<li><a href="#contact">Contact</a></li> -->
    <li><a href="#acknowledgments">Acknowledgments</a></li>
  </ol>
</details>

<!-- ABOUT THE PROJECT -->
## About The Project

MicroUSC (Micro Universal Serial Controller) is a framework that makes it easy to control an ESP32 using UART serial communication 
based strictly on 4-byte (long integer) commands. The main point of the project is to provide a fast, reliable, and memory-efficient 
way for another microcontroller to send compact binary commands—each exactly 4 bytes—that the ESP32 interprets to perform specific 
actions. This approach avoids the complexity and overhead of string-based protocols by using fixed-size binary messages, which are 
ideal for embedded systems needing speed and simplicity. microusc handles all the low-level UART driver setup, memory management, 
and synchronization, ensuring that each 4-byte command is received, decoded, and executed accurately, even in demanding or noisy 
environments.

# Developer's Notice
What each 4-byte command means, how it is interpreted, and what actions it triggers are left completely up to the developer using 
the library. MicroUSC does not prescribe any specific command set or behavior—instead, it provides a robust, flexible foundation 
that developers can adapt to their own application needs. This allows for maximum customization: you can define your own command 
codes, data structures, and responses, making microusc suitable for a wide range of embedded projects, from remote device control
to sensor data acquisition, all while benefiting from the library’s efficient and reliable UART handling.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- GETTING STARTED -->
## Getting Started

All requirements needed to use the open source code

### Prerequisites
To download the official ESP-IDF environment
* ESP-IDF [officialespressif-esp-idf](https://dl.espressif.com/dl/esp-idf/)
or download through the official ESP-IDF Microsoft Visual Code extension

### Installation
For Windows:
   * It is recommended to clone `OUTSIDE OF THE ONEDRIVER FOLDER` as it may cause
     issues.

     Example dirctory:
     ```sh
     C:\Users\your_username\MicroUSC
     ```
     Not
     ```sh
     C:Users\OneDrive\MicroUSC
     ```
     Then clone the repo
     ```sh
     git clone https://github.com/repvi/MicroUSC.git
     ```

For Linux:
   * Clone the repo
     ```sh
     git clone https://github.com/repvi/MicroUSC.git
     ```

That's it!

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- USAGE EXAMPLES -->
## Usage

Remote GPIO Control:
Use another microcontroller to send a 4-byte command to the ESP32 to set, clear, or toggle a specific GPIO pin. For example, a 
command like 0x0001000A could mean "set GPIO10 high," where the first byte is the command type and the remaining bytes encode 
the pin and state.

Sensor Data Requests:
The controller sends a 4-byte command requesting the ESP32 to read a sensor (e.g., temperature or light). The ESP32 interprets 
the command and responds with a 4-byte value representing the sensor reading.

Device Configuration:
Adjust settings on the ESP32, such as changing the sampling rate of a sensor or enabling/disabling features, by sending specific 
4-byte configuration commands.

Actuator Control:
Control motors, relays, or servos attached to the ESP32 by sending compact 4-byte commands that encode the actuator type, channel, 
and desired action.

Status Monitoring:
The ESP32 can periodically send back 4-byte status codes representing its current state, error flags, or other diagnostics, making
it easy for the controller to monitor system health.

Multi-Device Coordination:
In a distributed system, several ESP32s can be controlled from a central microcontroller, each receiving and acting on 4-byte 
commands for synchronized operations in robotics, automation, or sensor networks

<!--_For more examples, please refer to the [Documentation](https://example.com)_ -->

<p align="right">(<a href="#readme-top">back to top</a>)</p>


<!-- ROADMAP
## Roadmap

- [ ] Feature 1
- [ ] Feature 2
- [ ] Feature 3
    - [ ] Nested Feature

See the [open issues](https://github.com/repvi/MicroUSC/issues) for a full list of proposed features (and known issues).

<p align="right">(<a href="#readme-top">back to top</a>)</p>

-->

<!-- CONTRIBUTING -->
## Contributing

Contributions are what make the open source community such an amazing place to learn, inspire, and create. Any contributions you make are **greatly appreciated**.

If you have a suggestion that would make this better, please fork the repo and create a pull request. You can also simply open an issue with the tag "enhancement".
Don't forget to give the project a star! Thanks again!

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

<p align="right">(<a href="#readme-top">back to top</a>)</p>

### Top contributors:

<a href="https://github.com/repvi/MicroUSC/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=repvi/MicroUSC" alt="contrib.rocks image" />
</a>



<!-- LICENSE -->
## License

Distributed under the project_license. See `LICENSE.txt` for more information.

<p align="right">(<a href="#readme-top">back to top</a>)</p>


<!-- CONTACT
## Contact

username - [@twitter_handle](https://twitter.com/twitter_handle) - email@email_client.com

Project Link: [https://github.com/repvi/MicroUSC](https://github.com/repvi/MicroUSC)

<p align="right">(<a href="#readme-top">back to top</a>)</p>
-->


<!-- ACKNOWLEDGMENTS -->
## Acknowledgments

* This project utilizes libraries and components from Espressif Systems, including the ESP-IDF framework.
  ESP-IDF: Provides core drivers, FreeRTOS integration, and UART functionality.
  
  Licensed under the Apache License 2.0.

  Please refer to the ESP-IDF repository for detailed license information and source code.
[officialespressif](https://www.espressif.com)
<!--
* []()
* []()

-->
<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- MARKDOWN LINKS & IMAGES -->
<!-- https://www.markdownguide.org/basic-syntax/#reference-style-links -->
[contributors-shield]: https://img.shields.io/github/contributors/repvi/MicroUSC.svg?style=for-the-badge
[contributors-url]: https://github.com/repvi/MicroUSC/graphs/contributors
[forks-shield]: https://img.shields.io/github/forks/repvi/MicroUSC.svg?style=for-the-badge
[forks-url]: https://github.com/repvi/MicroUSC/network/members
[stars-shield]: https://img.shields.io/github/stars/repvi/MicroUSC.svg?style=for-the-badge
[stars-url]: https://github.com/repvi/MicroUSC/stargazers
[issues-shield]: https://img.shields.io/github/issues/repvi/MicroUSC.svg?style=for-the-badge
[issues-url]: https://github.com/repvi/MicroUSC/issues
[license-shield]: https://img.shields.io/github/license/repvi/MicroUSC.svg?style=for-the-badge
[license-url]: https://github.com/repvi/MicroUSC/blob/master/LICENSE.txt
[linkedin-shield]: https://img.shields.io/badge/-LinkedIn-black.svg?style=for-the-badge&logo=linkedin&colorB=555
[linkedin-url]: https://linkedin.com/in/alejandro-ramirez-893247310