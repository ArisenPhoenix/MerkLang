#include "core/node/NodeStructures.hpp"
#include "core/node/ArgumentNode.hpp"
#include "core/callables/classes/bultins.h"
#include "utilities/debugger.h"
#include "core/errors.h"
#include "algorithm"
#include "core/Evaluator.hpp"

#include <curl/curl.h>
#include <functional> 


static String getStringField(SharedPtr<ClassInstance> inst, const char* name) {
    auto n = inst->getField(name);
    if (!n.isValid() || !n.isString()) throw MerkError(String("Http: '") + name + "' must be a String");
    return n.toString();
}

static SharedPtr<DictNode> getDictField(SharedPtr<ClassInstance> inst, const char* name) {
    auto n = inst->getField(name);
    if (!n.isValid() || !n.isDict())
        throw MerkError(String("Http: '") + name + "' must be a Dict, but is a " + nodeTypeToString(DynamicNode::getTypeFromValue(n.getValue())));
    return n.toDict(); 
}


size_t writeToString(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    String* str = static_cast<String*>(userp);
    str->append(static_cast<char*>(contents), totalSize);
    return totalSize; 
}


// Convert your DictNode -> curl_slist* (remember to free)
static curl_slist* dictToCurlHeaders(const DictNode& dict) {
    curl_slist* list = nullptr;
    for (auto& [k,v] : dict.getElements()) {
        String line = k.toString();
        line += ": ";
        line += v.toString();
        list = curl_slist_append(list, line.c_str());
    }
    return list;
}




HttpNode::HttpNode() = default;
HttpNode::HttpNode(SharedPtr<ClassInstanceNode> instanceNode): InstanceBoundNative(instanceNode) {
    auto dictArgs = ArgumentList();
    auto dictType = ResolvedType("Dict");
    auto dictFlags = DataTypeFlags(false, true, true, dictType);
    auto instance = getInstance();
    auto self = instance->getInstanceNode();
    auto instanceScope = self->getInstanceScope();

    auto dictClassOpt = instanceScope->getClass("Dict");
    if (!dictClassOpt.has_value()) { throw MerkError("Class Dict Did Not Exist When Constructing NativeHttpClass");}
        
    auto headersNode = Evaluator::evaluateClassCall(instanceScope, "Dict", dictArgs, self);
    auto headers = VarNode(headersNode, dictFlags);
    
    auto bodyNode = Evaluator::evaluateClassCall(instanceScope, "Dict", dictArgs, self);
    auto body = VarNode(bodyNode, dictFlags);

    instance->declareField("headers", headers);

    instance->declareField("body", body);

    instance->declareField("status", Node(0));
    instance->declareField("ok", Node(false));
}
HttpNode::~HttpNode() {}


Node HttpNode::send() {
    auto inst = getInstance();
    if (!inst) throw MerkError("HttpNode has no bound instance");

    
    auto url    = getStringField(inst, "url");
    auto method = getStringField(inst, "method");

    auto headersDict = getDictField(inst, "headers");
    auto bodyDict    = getDictField(inst, "body");

    // If you want to support raw string body too, consider letting body be String or Dict.
    // For now, serialize Dict to JSON-ish or x-www-form-urlencoded.
    String bodyStr;
    // TODO: pick a content-type policy; example below is naive JSON-ish:
    // bodyStr = dictToJson(*bodyDict); // implement or plug your existing Dict->String

    CURL* curl = curl_easy_init();
    if (!curl) throw MerkError("curl_easy_init failed");

    String response;
    long status = 0;
    curl_slist* hdrList = nullptr;

    try {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        // headers
        hdrList = dictToCurlHeaders(*headersDict);
        if (hdrList) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrList);

        // method
        auto m = method; 
        for (auto& c : m) c = std::toupper(static_cast<unsigned char>(c));
        if (m == "GET") {
            curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
        } else if (m == "POST") {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, bodyStr.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, bodyStr.size());
        } else if (m == "PUT" || m == "PATCH" || m == "DELETE") {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, m.c_str());
            if (m != "DELETE") {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, bodyStr.c_str());
                curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, bodyStr.size());
            }
        } else {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, m.c_str());
        }

        // perform
        auto rc = curl_easy_perform(curl);
        if (rc != CURLE_OK) {
            throw MerkError(String("HTTP error: ") + curl_easy_strerror(rc));
        }
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    } catch (...) {
        if (hdrList) curl_slist_free_all(hdrList);
        curl_easy_cleanup(curl);
        throw;
    }

    if (hdrList) curl_slist_free_all(hdrList);
    curl_easy_cleanup(curl);

    // Build response Dict Node
    auto scope = inst->getInstanceScope(); // for constructing nodes

    // headers out: libcurl doesn’t give easy header map via easy API. If you need response headers:
    // use CURLOPT_HEADERFUNCTION to capture them into a temporary map<str,str> and build a DictNode here.

    auto respDictArgs = ArgumentList();
    auto respNode = Evaluator::evaluateClassCall(scope, "Dict", respDictArgs, nullptr);
    auto resp = respNode.toInstance();
    // auto resp = std::static_pointer_cast<DictNode>(respNode.toInstance()->getNativeData());

    // inst->updateField("status", Node(static_cast<int>(status)));
    // inst->updateField("ok", Node(status >= 200 && status < 300));
    // inst->updateField("body", Node(response));
    
    // DEBUG_LOG(LogLevel::PERMISSIVE, "RESP: " + resp->toString());
    // throw MerkError("GOt Response");
    // return Node(resp); 
    // return inst->getInstanceNode()->getInstanceNode();
    auto respDict = std::static_pointer_cast<DictNode>(resp->getNativeData());
    // DEBUG_LOG(LogLevel::PERMISSIVE, "RESP: " + resp->toString());
    // auto stat = ;
    // if (stat.isBool()) {throw MerkError("Status is a bool");}
    respDict->set("status", Node(static_cast<int>(status)));
    respDict->set("ok",     Node(status >= 200 && status < 300));
    respDict->set("body",   Node(response));
    auto resInstance = respNode.toInstance();
    resInstance->declareField("body", respDict->get("body"));
    resInstance->declareField("ok", respDict->get("ok"));
    resInstance->declareField("status", respDict->get("status"));
    // resp->set(Node("headers"), headersDictFromResponse);
    // DEBUG_LOG(LogLevel::PERMISSIVE, "RESP 2: " + resp->toString());
    // DEBUG_LOG(LogLevel::PERMISSIVE, "Dict Data: " + respDict->toString());
    // throw MerkError("Got Response");
    if (!respNode.isInstance()) {throw MerkError("The response is not an instance");}
    return respNode;

    
    
    // return Node();

} // performs the HTTP request and returns a response Node

String HttpNode::toString() const {
    return "";
}
std::size_t HttpNode::hash() const {
    return 0;
}

VariantType HttpNode::getValue() const {
    return "NOTHING TO PULL YET";
};
void HttpNode::setValue(const VariantType& v) {
    throw MerkError("Tried to setValue on Http" + nodeTypeToString(DynamicNode::getTypeFromValue(v)));
}

bool HttpNode::holdsValue() { return true; }

SharedPtr<NodeBase> HttpNode::clone() const {
    return makeShared<HttpNode>();
}
void HttpNode::clear() {
    if (getInstance()) {
        getInstance()->clear();
    }
    
}











void FileNode::loadConfigFromInstance() {
    auto inst = getInstance(); // ClassInstance
    const auto path = inst->getField("path").toString();
    const auto modeStr = inst->getField("mode").toString(); // e.g. "r","wb"

    std::ios::openmode m{};
    binary = modeStr.find('b') != String::npos;
    if (binary) m |= std::ios::binary;

    if (modeStr.find('r') != String::npos) m |= std::ios::in;
    if (modeStr.find('w') != String::npos) m |= (std::ios::out | std::ios::trunc);
    if (modeStr.find('a') != String::npos) m |= (std::ios::out | std::ios::app);

    stream.open(path, m);
    if (!stream.is_open()) throw MerkError("File.open failed: " + path + " (" + modeStr + ")");
}

void FileNode::open() {
    if (isOpen()) return;
    loadConfigFromInstance();
}

void FileNode::close() { closeQuiet(); }

String FileNode::readAll() {
    if (!isOpen()) open();
    std::ostringstream oss;
    oss << stream.rdbuf();
    return oss.str();
}

String FileNode::read(size_t n = 0) {
    if (n == 0) {
        return readAll();
    }
    if (!isOpen()) open();
    String s(n, '\0');
    stream.read(&s[0], static_cast<std::streamsize>(n));
    s.resize(static_cast<size_t>(stream.gcount()));
    return s;
}

void FileNode::write(String s) {
    if (!isOpen()) open();
    stream.write(s.data(), static_cast<std::streamsize>(s.size()));
    if (!stream) throw MerkError("File.write failed");
}

void FileNode::writeBytes(const std::vector<uint8_t>& b) {
    if (!isOpen()) open();
    stream.write(reinterpret_cast<const char*>(b.data()), static_cast<std::streamsize>(b.size()));
    if (!stream) throw MerkError("File.writeBytes failed");
}

void FileNode::seek(std::streamoff pos, std::ios_base::seekdir dir) {
    if (!isOpen()) open();
    stream.clear();
    stream.seekg(pos, dir);
    stream.seekp(pos, dir);
}

std::streamoff FileNode::tell() {
    if (!isOpen()) open();
    return stream.tellg(); // for text reads this is fine; could prefer tellp when writing
}

bool FileNode::exists(const String& path) { return std::filesystem::exists(path); }
std::uintmax_t FileNode::sizeOf(const String& path) { return std::filesystem::file_size(path); }
void FileNode::removeFile(const String& path) {
    std::error_code ec;
    std::filesystem::remove(path, ec);
    if (ec) throw MerkError("File.remove failed: " + ec.message());
}
