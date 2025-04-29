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


#ifndef CURL_CPP_HPP_
#define CURL_CPP_HPP_

#include "curl_cpp_errors.hpp"

#include <string>
#include <ostream>
#include <type_traits>

#include <curl/curl.h>

///USAGE : everything is in the curl_cpp namespace
/// curl_get([Curl_handle], url, append_here);
///   [Curl_handle] is optional, use it to set curl options
///   url is a const char* or a const std::string&
///   append_here is a std::string& or an ostream&, get data goes here
///
/// curl_post([Curl_handle], url, post_me);
///   [Curl_handle] is optional, use it to set curl options
///   url     is a const char* or a const std::string&
///   post_me si a const char* or a const std::string&.
///
/// curl_post_get([Curl_handle], url, data, append_here);
///   [Curl_handle] is optional, use it to set curl options
///   url         is a const char* or a const std::string&
///   post_me     si a const char* or a const std::string&.
///   append_here is a std::string& or an ostream&, get data goes here
///
/// Set curl options :
///   Curl_handle h;
///   curl_easy_setopt(h,CURL_OPTION,...);
///   NOTE : h should NOT be recycled among multiple calls to curl_get and curl_post.
///


typedef void CURL;


namespace curl_cpp{

//================================
//=== Convert stuff to cstring ===
//================================

template<typename T>
struct To_cstring_t{static constexpr bool value = false;};

template<typename T>
std::enable_if_t< To_cstring_t<T>::value ,const char*  >
to_cstring(const T &t){return To_cstring_t<T>::run(t);}

//specialize (user may specialize)
template<>
struct To_cstring_t<std::string>{
    static constexpr bool value = true;
    static const char* run(const std::string &s){return s.c_str();}
};

template<>
struct To_cstring_t<const char*>{
    static constexpr bool value = true;
    static const char* run(const char* s){return s;}
};

template<size_t N>
struct To_cstring_t<char[N]>{
    static constexpr bool value = true;
    static const char* run(const char* s){return s;}
};




//=========================
//=== RAII Curl_handle  ===
//=========================

struct Curl_handle{
    Curl_handle();
    ~Curl_handle();

    //movable, not copiable
    Curl_handle(Curl_handle&&)=default;
    Curl_handle(const Curl_handle&)=delete;
    Curl_handle& operator=(const Curl_handle&)=delete;

    CURL* curl=nullptr;
    operator CURL*(){return curl;}
    CURL* get()     {return curl;}
};


struct Curl_slist_handle{
    Curl_slist_handle();
    ~Curl_slist_handle();

    //movable, not copiable
    Curl_slist_handle(Curl_slist_handle&&)=default;
    Curl_slist_handle(const Curl_slist_handle&)=delete;
    Curl_slist_handle& operator=(const Curl_slist_handle&)=delete;

    void append(const char*);
    void append(const std::string &s){append(s.c_str());}

    curl_slist *slist=nullptr;
    operator curl_slist*(){return slist;}
    curl_slist* get()     {return slist;}

};

//====================
//=== curl_receive ===
//====================

//The user may specialize this interface to tell curl how to write curl output in various objects

template<typename T, typename Enable=void> struct Curl_receive_t{
    Curl_receive_t()=delete;
    static constexpr bool value =false;
};

template<>
struct Curl_receive_t<std::string>{
    Curl_receive_t()=delete;
    static constexpr bool value =true;

    struct Curl_wrap_string{
        std::string& out;
        Curl_wrap_string(std::string &out_):out(out_){};
    };

    typedef std::string      written_type;
    typedef Curl_wrap_string prepared_type;

    static prepared_type prepare(Curl_handle &curl, const char* url,  written_type &append_here);
    static size_t        receive(void *ptr, size_t size, size_t nmemb, void *stream)noexcept;
    static void          finish ( Curl_handle &curl, const char* url, written_type &append_here, prepared_type &p);
};



template<>
struct Curl_receive_t<std::ostream>{
    Curl_receive_t()=delete;
    static constexpr bool value =true;

    struct Curl_wrap_ostream{
        std::ostream &out;
        std::string  err;
        Curl_wrap_ostream(std::ostream &out_):out(out_){};
        //operator void*(){return static_cast<void*>(this);}
    };
    typedef std::ostream      written_type;
    typedef Curl_wrap_ostream prepared_type;

    static Curl_wrap_ostream prepare(Curl_handle &curl, const char* url, written_type&);
    static size_t            receive(void *ptr, size_t size, size_t nmemb, void *stream)noexcept;
    static void              finish ( Curl_handle &curl, const char* url, written_type &append_here, prepared_type &p);
};

//use it for any ostream derivate
template<typename T>
struct Curl_receive_t<
        T,
        typename std::enable_if< std::is_base_of<std::ostream,T>::value and not std::is_same<std::ostream,T>::value >::type
>:Curl_receive_t<std::ostream>
{};





//=================
//=== curl_send ===
//=================

//The user may specialize this interface to tell curl how to send (i.e., post) data from various objects

template<typename T> struct Curl_send_t{
    Curl_send_t()=delete;
    static constexpr bool value =false;
};

template<>
struct Curl_send_t<std::string>{
    Curl_send_t()=delete;
    static constexpr bool value =true;

    static void send   (Curl_handle &curl, const char* url, const std::string &send_me);
    static void finish (Curl_handle &curl, const char* url, const std::string &send_me);
};

template<>
struct Curl_send_t<const char*>{
    Curl_send_t()=delete;
    static constexpr bool value =true;

    static void send   (Curl_handle &curl, const char* url, const  char*  send_me);
    static void finish (Curl_handle &curl, const char* url, const  char*  send_me);
};


template<size_t N>
struct Curl_send_t<char[N]>:Curl_send_t<const char*>{};






//================
//=== curl_get ===
//================
namespace details{
    //bury curl specific code here
    void curl_get_impl(Curl_handle &curl, const char* &url, void* append_here, size_t(*fn)(void *,size_t,size_t,void*) );

    //prepare and get
    template<typename T>
    std::enable_if_t< Curl_receive_t<T>::value >
    curl_get_t(Curl_handle &curl, const char* url, T  &append_here){
        auto p = Curl_receive_t<T>::prepare(curl,url,append_here);
        curl_get_impl(curl, url, static_cast<void*>(&p),  Curl_receive_t<T>::receive );
        Curl_receive_t<T>::finish(curl,url,append_here,p);
    }
}

//--- GET interface ---
//std::enable_if_t<   curl_cpp::details::Is_string<Url_t>::value   >
template<typename Url_t , typename App_t >
std::enable_if_t<To_cstring_t<Url_t>::value and Curl_receive_t<App_t>::value >
curl_get(Curl_handle &h, const Url_t &url, App_t &append_here){
    const char * u = curl_cpp::to_cstring( url);
    details::curl_get_t(h,u, append_here);
}

template<typename Url_t , typename App_t >
std::enable_if_t< To_cstring_t<Url_t>::value and Curl_receive_t<App_t>::value >
curl_get(const Url_t &url, App_t &append_here){
    Curl_handle h;
    const char * u = curl_cpp::to_cstring( url);
    details::curl_get_t(h, u  , append_here);
}




//=================
//=== curl_post ===
//=================
/*
template<>
struct Curl_send_t<const char*>{
    static constexpr bool value =true;
    static const char* prepare(const char* send_me){return send_me;}
    static void send(Curl_handle &curl,const char * send_me);
};
*/

namespace details{
    //prepare and get
    template<typename T>
    std::enable_if_t< Curl_send_t<T>::value >
    curl_post_t(Curl_handle &curl, const char* url, const T  &send_me){
        //curl_post_impl(curl, url, static_cast<void*>() );
        Curl_send_t<T>::send  (curl,url,send_me);
        Curl_send_t<T>::finish(curl,url,send_me);
    }

}


//--- curl_post interface ---

template<typename Url_t, typename Send_t>
std::enable_if_t<
  To_cstring_t<Url_t>::value and
  Curl_send_t<Send_t>::value
>
curl_post(const Url_t &url, const Send_t &data){
    Curl_handle h;
    const char* u = curl_cpp::to_cstring(url);
    details::curl_post_t(h, u, data );
}

template<typename Url_t, typename Send_t>
std::enable_if_t<
  To_cstring_t<Url_t>::value and
  Curl_send_t<Send_t>::value
>
curl_post(Curl_handle &h, const Url_t &url, const Send_t &data){
    const char* u = curl_cpp::to_cstring(url);
    details::curl_post_t(h, u, data );
}


//=====================
//=== curl_post_get ===
//=====================
namespace details{
    //prepare and get
    template<typename Send_t, typename Receive_t>
    std::enable_if_t< Curl_send_t<Send_t>::value and Curl_receive_t<Receive_t>::value >
    curl_post_get_t(Curl_handle &curl, const char* url, const Send_t  &send_me, Receive_t& append_here ){
        //curl_post_impl(curl, url, static_cast<void*>() );
        Curl_send_t<Send_t>::send  (curl,url,send_me);
        auto p = Curl_receive_t<Receive_t>::prepare(curl,url,append_here);
        curl_get_impl(curl, url, static_cast<void*>(&p),  Curl_receive_t<Receive_t>::receive );
        Curl_send_t<Send_t>::finish(curl,url,send_me);
    }
}



//--- curl_post_get interface ---

template<typename Url_t, typename Send_t, typename Receive_t>
std::enable_if_t<
  To_cstring_t<Url_t>::value and
  Curl_send_t<Send_t>::value and
  Curl_receive_t<Receive_t>::value
>
curl_post_get(const Url_t &url, const Send_t &data, Receive_t &receive){
    static_assert(std::is_reference <decltype(url) >::value,"" );
    static_assert(std::is_reference <decltype(data)>::value,"" );

    Curl_handle h;
    const char* u = curl_cpp::to_cstring(url);

    details::curl_post_get_t(h, u, data,receive );
}

template<typename Url_t, typename Send_t, typename Receive_t>
std::enable_if_t<
  To_cstring_t<Url_t>::value and
  Curl_send_t<Send_t>::value and
  Curl_receive_t<Receive_t>::value
>
curl_post_get(Curl_handle &h, const Url_t &url, const Send_t &data, Receive_t &receive){
    static_assert(std::is_reference <decltype(url) >::value,"" );
    static_assert(std::is_reference <decltype(data)>::value,"" );

    const char* u = curl_cpp::to_cstring(url);
    details::curl_post_get_t(h, u, data,receive );
}

/*
template<typename Url_t, typename Send_t, typename Receive_t>
void curl_post_get_test(const Url_t &url, const Send_t &data, Receive_t &receive){
    static_assert(To_cstring_t<Url_t>::value,"1");
    static_assert(Curl_send_t<Send_t>::value,"2");
    static_assert(Curl_receive_t<Receive_t>::value,"3");
}
*/


}//end namespace curl_cpp


#endif
