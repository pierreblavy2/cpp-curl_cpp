/*
Copyright (C) 2024 Pierre BLAVY

This program (curl_cpp) is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

//This program uses curl, see curl.se

#ifndef CURL_CPP_ERRORS_HPP
#define CURL_CPP_ERRORS_HPP


#include <stdexcept>


namespace curl_cpp{

//--- ERRORS ---
//Base error in curl, ex :
//  - cannot initialise curl handle
//  - bad curl result
struct Curl_error:std::runtime_error{
    typedef std::runtime_error base_type;
    using base_type::base_type;
    Curl_error(const std::string&s, const char* url):base_type(s + ", url="+url){}
};

//HTTP Error
struct Curl_error_http:Curl_error{
    typedef Curl_error base_type;
    using base_type::base_type;
    long error_number = 0; //0=undefined
};


}

#endif // CURL_CPP_ERRORS_HPP
