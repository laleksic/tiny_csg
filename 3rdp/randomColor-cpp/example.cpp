/******************************************************************************
 * Copyright (c) 2015 Artur Eganyan
 *
 * This software is provided "AS IS", WITHOUT ANY WARRANTY, express or implied.
 ******************************************************************************/

#include "randomcolor.h"
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <iomanip>

// Create and open html with all color ranges
int main( int argc, char** argv )
{
    RandomColor randomColor;
    const char* luminosityNames[] = {"Dark", "Normal", "Light", "Bright", "Random"};
    const char* colorNames[] = {
        "Red", "RedOrange", "Orange", "OrangeYellow", "Yellow",
        "YellowGreen", "Green", "GreenCyan", "Cyan", "CyanBlue",
        "Blue", "BlueMagenta", "Magenta", "MagentaPink", "Pink",
        "PinkRed", "RandomHue", "BlackAndWhite", "Brown"
    };

    std::fstream file("example.html", std::fstream::out | std::fstream::trunc);
    if (file.fail()) {
        std::cerr << "Can not open example.html" << std::endl;
        return EXIT_FAILURE;
    }

    auto addRow = [&]( RandomColor::Color color, RandomColor::Luminosity luminosity )
    {
        // This resets the random generator for each row, so that rows will
        // contain identically distributed colors. To generate random rows,
        // it should be commented out.
        randomColor.setSeed(123); // This value is just for test (not even a prime)

        file << "<tr>";
        file << "<td style='width:100px; text-align:right;'>" << luminosityNames[int(luminosity)] << "&nbsp&nbsp</td>";
        file << "<td style='line-height:0px;'>";
        for (int i = 0; i < 10; ++ i) {
            file << "<div style='width:50px; height:50px; margin:2px; display:inline-block; background-color:#"
                 << std::hex << std::setw(6) << std::setfill('0')
                 << randomColor.generate(color, luminosity)
                 << "'></div>";
        }
        file << "</td></tr>";
    };

    file << "<!DOCTYPE html><html><body>";
    file << "<table style='width:750px; margin:auto;'>";

    for (int c = RandomColor::Red; c <= RandomColor::Brown; ++ c) {
        file << "<tr><th colspan='2' style='text-align:center;'>" << colorNames[c] << "</th></tr>";
        addRow((RandomColor::Color)c, RandomColor::Light);
        addRow((RandomColor::Color)c, RandomColor::Normal);
        addRow((RandomColor::Color)c, RandomColor::Dark);
        addRow((RandomColor::Color)c, RandomColor::Bright);
        addRow((RandomColor::Color)c, RandomColor::RandomLuminosity);
        file << "<tr style='height:50px;'></tr>";
    }

    file << "</table></body></html>";
    file.close();

    system("example.html");
    return EXIT_SUCCESS;
}
