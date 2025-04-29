# cpp-curl_cpp
Curl c++ wrapper

(c) Pierre BLAVY 2024, [LGPL 3.0](https://www.gnu.org/licenses/lgpl-3.0.txt)


# Get a page
## Example
```c++
curl_cpp::Curl_handle curl;
//set options (see "Set curl options")

std::string page;
curl_cpp::curl_get(curl, "http://google.com" , page);
```

## Functions
* `curl_get([Curl_handle], url, append_here)` append the page content in append_here
* `curl_post([Curl_handle], url, post_me)` post the content of post_me to url
* `curl_post_get([Curl_handle], url, data, append_here)` post the content of post_me to url, and append the content of the server answer to append_here.

| Parameter       | Description |
| --------------- | ------------- |
| [Curl_handle] ` | A curl handle. This parameter is optional, use it to set curl options  |
|`url`            | The page URL. A const char* or a const std::string& |
|`post_me`        | The content to post. A const char* or a const std::string&. You can specialize `curl_cpp::Curl_send_t<MyType>` to add custom types support. |
|`append_here`    | Where to append the page returned by the server. A std::string& or a std::ostream&. You can specialize `curl_cpp::Curl_receive_t<MyType>` to add custom types support. |


# Set curl options
## Example
```c++
curl_cpp::Curl_handle curl;
auto err = curl_easy_setopt(curl,CURL_OPTION,...);
if ( err != CURLE_OK  ){
  // something wrong, see
  // https://curl.se/libcurl/c/libcurl-errors.html
}
```

* The curl options are documented [here](https://curl.se/libcurl/c/curl_easy_setopt.html)
* The curl errors are documented [here](https://curl.se/libcurl/c/libcurl-errors.html)
