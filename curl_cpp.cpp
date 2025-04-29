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



#include "curl_cpp.hpp"
#include <curl/curl.h>


using namespace curl_cpp;



//--- error throwing helpers ---
namespace{
    inline void curl_res_throw(CURLcode res, const std::string &prefix, const char * url){
        if(res != CURLE_OK){
            std::string err = curl_easy_strerror(res);
            throw curl_cpp::Curl_error(prefix+", message="+err , url);
        }
    }

    inline void curl_http_throw(CURL* curl, const std::string &prefix, const char* url){
        long http_code = 0;
        curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
        if(http_code != 200){
             curl_cpp::Curl_error_http err (prefix+", http_error="+std::to_string(http_code) , url);
             err.error_number = http_code;
             throw err;

        }
    }

    inline void curl_throw(CURL* curl, CURLcode res, const std::string &prefix, const char * url){
        curl_res_throw (res,prefix,url);
        curl_http_throw(curl,prefix,url);
    }
}//end namespace { for error throwing helpers



//=== Curl_handle ===

curl_cpp::Curl_handle:: Curl_handle(){
    curl = curl_easy_init();
    if(!curl){throw Curl_error("ERROR in curl : cannot initialize curl");}
}

curl_cpp::Curl_handle:: ~Curl_handle(){
    curl_easy_cleanup(curl);
}

//=== Curl_slist_handle ===

curl_cpp::Curl_slist_handle:: Curl_slist_handle(){}

curl_cpp::Curl_slist_handle:: ~Curl_slist_handle(){
    curl_slist_free_all(slist);
}

void curl_cpp::Curl_slist_handle::append(const char*s){
    slist = curl_slist_append(slist,s);
}






//=== Receive in a std::string ===
auto Curl_receive_t<std::string>::prepare(Curl_handle &, const char*, written_type&w)->prepared_type{
  return Curl_wrap_string(w);
}

size_t Curl_receive_t<std::string>::receive(void *ptr, size_t size, size_t nmemb, void *stream)noexcept{
   //ptr   = downloaded chunk
   //size = 1
   //nmemb = size of downloaded chunk
   //stream* = write_here

   Curl_wrap_string *w = static_cast<Curl_wrap_string*>(stream);
   const char* ptr_c = static_cast<char*>(ptr);
   w->out.append(ptr_c, nmemb);
   return size * nmemb;
}


void Curl_receive_t<std::string>::finish ( Curl_handle &curl, const char*url , written_type&, prepared_type &){
    CURLcode res = curl_easy_perform(curl);
    curl_throw(curl,res,"ERROR in curl get to string",url);
}






//=== Receive in a std::ostream ===
auto Curl_receive_t<std::ostream>::prepare(Curl_handle &, const char*, written_type &w)->Curl_wrap_ostream{
    Curl_wrap_ostream r(w);
    return r;
}



size_t Curl_receive_t<std::ostream>::receive(void *ptr, size_t size, size_t nmemb, void *stream)noexcept {
        //ptr   = downloaded chunk
        //size = 1
        //nmemb = size of downloaded chunk
        //stream* = write_here
       Curl_wrap_ostream * here = static_cast<Curl_wrap_ostream*>(stream);
       const char* ptr_c        = static_cast<char*>(ptr);

       if(!here->out){here->err = "invalid input ostream"; return (size*nmemb)+1;} //any return different from size * nmemb is an error

       std::string buffer; buffer.reserve(nmemb);
       buffer.append(ptr_c, nmemb);

       try{
         here->out << buffer;
       }
       catch(std::exception &e){here->err = e.what();  return (size*nmemb)+1;}
       catch(...){here->err="cannot write to ostream"; return (size*nmemb)+1;}

       if(!here->out){here->err = "invalid ostream after write"; return (size*nmemb)+1;} //any return different from size * nmemb is an error
       return size * nmemb;
};


void Curl_receive_t<std::ostream>::finish(  Curl_handle &curl, const char* url, written_type &, prepared_type &p ){
    if(p.err!=""){
        throw Curl_error("ERROR in curl get to ostream, message="+p.err+", url="+url);
    }

    CURLcode res = curl_easy_perform(curl);
    curl_throw(curl,res,"ERROR in curl get to ostream",url);
}




//=== GET ===

//================
//=== curl_get ===
//================
void details::curl_get_impl(Curl_handle &curl, const char* &url, void* append_here, size_t(*fn)(void *,size_t,size_t,void*) ){
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); //allow redirect
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fn);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, append_here);
}


//=== GET ===
/*
template<typename T>
std::enable_if_t< Curl_receive_t<T>::value >
curl_get_t(Curl_handle &curl, const char* &url, T  &append_here){
    auto p = Curl_receive_t<T>::prepare(curl,url,append_here);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); //allow redirect
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Curl_receive_t<T>::receive);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &p);

    Curl_receive_t<T>::finish(curl,url,append_here,p);

}*/

/*
template<typename T>
std::enable_if_t< Curl_receive_t<T>::value >
curl_get_t(Curl_handle &curl, const char* &url, T  &append_here){
    auto p = Curl_receive_t<T>::prepare(curl,url,append_here);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); //allow redirect
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Curl_receive_t<T>::receive);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &p);

    Curl_receive_t<T>::finish(curl,url,append_here,p);

}*/
/*
void Curl_get_t<std::ostream>::run(Curl_handle &h, const char* url, std::ostream   &append_here){
    curl_get_t(h,url,append_here);
}

void Curl_get_t<std::string>::run(Curl_handle &h, const char* url, std::string   &append_here){
    curl_get_t(h,url,append_here);
}

*/

//=== POST ===


void Curl_send_t<std::string>::send(Curl_handle &curl, const char* url, const std::string &data){
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str() );
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.size() ); //USE string size instead of default C strlen, as string may contains '\0'
}

void  Curl_send_t<std::string>::finish ( Curl_handle &curl, const char* url, const std::string &){
    CURLcode res = curl_easy_perform(curl);
    curl_throw(curl,res,"ERROR in curl post string",url);
}



void Curl_send_t<const char*>::send(Curl_handle &curl,const char* url, const char *data){
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data );
}

void  Curl_send_t<const char*>::finish ( Curl_handle &curl, const char* url, const char *){
    CURLcode res = curl_easy_perform(curl);
    curl_throw(curl,res,"ERROR in curl post const char*",url);
}



namespace{
//TEST CODE
[[maybe_unused]] void must_compile(){
    const char * ct="";
    const std::string s;
    std::string out;
    curl_get(s,out);
    curl_get(ct,out);

    curl_post(s,s);
    curl_post(ct,s);
    curl_post(s,ct);
    curl_post(ct,ct);

    curl_post_get(s,s,out);
    curl_post_get(s,ct,out);
    curl_post_get(ct,s,out);
    curl_post_get(ct,ct,out);


    to_cstring(s);
    to_cstring(ct);
    to_cstring("");


    //To_cstring_t<Url_t>::value and
    //Curl_send_t<Send_t>::value and
    //Curl_receive_t<Receive_t>::value

}
}


/*
//post a const char *
void curl_cpp::curl_post(const char* url, const char* data){
    Curl_handle h;
    curl_post(h,url,data);
}

void curl_cpp::curl_post( Curl_handle &curl, const char* url, const char* data){
   //doc : https://curl.se/libcurl/c/simplepost.html

   curl_easy_setopt(curl, CURLOPT_URL, url);
   curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

   CURLcode res = curl_easy_perform(curl);
   curl_throw(curl,res,"ERROR in curl post",url);
}



//post a std::string
void curl_cpp::curl_post(const char* url, const std::string &data){
    Curl_handle h;
    curl_post(h,url,data);
}

void curl_cpp::curl_post(Curl_handle &curl, const char* url, const std::string &data){
   //doc : https://curl.se/libcurl/c/simplepost.html
   curl_easy_setopt(curl, CURLOPT_URL, url);
   curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str() );
   curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.size() ); //USE string size instead of default C strlen, as string may contains '\0'

   CURLcode res = curl_easy_perform(curl);
   curl_throw(curl,res,"ERROR in curl post",url);
}







*/
