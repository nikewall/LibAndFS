#include <iostream>
#include <regex>
#include <fstream>
#include <string.h>
#include "Wad.h"

using namespace std;

// Helper fn
int32_t readInt(istream &stream){
    uint8_t binStr[4];
    stream.read((char*) binStr, 4);

    return static_cast<int32_t>(
        (binStr[0])       |
        (binStr[1] << 8)  |
        (binStr[2] << 16) |
        (binStr[3] << 24) );
}

void populateTree(istream &stream, Node* node, int &numDesc) {
    while (numDesc > 0) {
        // read 16B into: 4B-offset, 4B-length, 8B-name
        int elOffset = (int) readInt(stream);
        int elLength = (int) readInt(stream);

        char elName_c[8];
        stream.read(elName_c, 8);
        string elName(elName_c);
        numDesc--;


        int found = elName.find("_START");
        if (found != string::npos){
            // get substring from the beginning to the _START
            elName = elName.substr(0, found);
        }
        // if is NOT an end of dir
        regex endRegex("^\\w+(_END)$");
        if (!(regex_match(elName, endRegex))){
            // check for E#M# marker
            regex emregex("^E\\dM\\d$");
            if (regex_match(elName, emregex)) {
                // is E#M# marker
                Node* newNode = new Node(elLength, elOffset, elName);
                node->children.push_back(newNode);

                // get 10 children of E#M# into E#M#
                int numEMs = 10;
                numDesc = numDesc - 10;
                populateTree(stream, newNode, numEMs);
            } else {
                // add this node to the children list of `node`
                Node* newNode = new Node(elLength, elOffset, elName);
                node->children.push_back(newNode);

                // if length == 0, this is a directory, recurse with this node
                if (elLength == 0) {
                    populateTree(stream, newNode, numDesc);
                } 
            }
        } else return; // is an end of dir, backtrack
    }
}

// Node implementation
Node::Node(int length, int offset, string name, list<Node*> children) {
    this->length = length;
    this->offset = offset;
    this->name = name;
    this->children = children;
}

// Wad implementation
Wad::Wad(string magic, int numDesc, int descOffset, Node* root, char* file) {
    this->magic = magic;
    this->numDesc = numDesc;
    this->descOffset = descOffset;
    this->root = root;
    this->data = file;
}

Wad* Wad::loadWad(const string &path){
    // load file from `path` into fstream
    ifstream stream(path, ios::binary);

    // get magic from first 4B of file
    char magic[4];
    stream.read(magic, 4);

    // get num of descriptors and descriptor block offset from next 8B 
    // safe to cast, since int32_t is alias of int
    int numDescriptors = (int)readInt(stream);
    int descOffset = (int)readInt(stream);

    // traverse to beginning of descriptors
    stream.seekg(descOffset, stream.beg);

    // Create and populate tree of children
    Node* rootNode = new Node(0, 0, string("/"));
    populateTree(stream, rootNode, numDescriptors);


    // after tree is populated, read whole file data into a C-string buffer
    stream.seekg(0, stream.end);
    int size = stream.tellg(); // get size of file in Bytes (UNIX at least ??)
    char file[size]; // make buffer of size len(file)
    stream.seekg(0, stream.beg);
    stream.read(file, size); // read into the buffer all bytes in file

    // Make a `new` Wad
    Wad* dynWad = new Wad(string(magic), numDescriptors, descOffset, rootNode, file);

    return dynWad;
}
string Wad::getMagic(){
    return this->magic;
}

list<string> parsePath(string path) {
    string pathStr = (string) path;

    // strip root "/" char
    string root = pathStr.substr(0,1);
    pathStr.erase(0,1);
    
    // split string into element names
    size_t pos = 0;
    list<string> split_str;
    split_str.push_back(root);

    while ((pos = pathStr.find("/")) != string::npos) {
        split_str.push_back(pathStr.substr(0, pos));
        pathStr.erase(0, pos + 1);
    }

    if (pathStr.length() > 0) {
        split_str.push_back(pathStr);
    }

    return split_str;
}

Node* locateNode(list<string> path, Node* node) {
    if (node->name.compare(path.front()) == 0) {
        path.pop_front();
        if (path.size() == 0) {
            // end of path, found node
            return node;
        } else {
            if (node->children.size() > 0) { 
                for (Node* child : node->children) {
                    Node* found = locateNode(path, child);
                    if (found != nullptr) {
                        return found;
                    }
                }
                return nullptr;
            } else {
                // node is not the node we wanted, and no children
                // is invalid path
                return nullptr;
            }
        }
    } else return nullptr; // invalid path
}

bool Wad::isContent(const string &path) {
    list<string> parsed_path = parsePath(string(path));
    Node* foundNode = locateNode(parsed_path, this->root);
    if (foundNode != nullptr) {
        if (foundNode->length > 0){
            return true;
        }
        else 
            return false;
    } else return false;
}
bool Wad::isDirectory(const string &path) {
    list<string> parsed_path = parsePath(string(path));
    Node* foundNode = locateNode(parsed_path, this->root);
    if (foundNode != nullptr) {
        if (foundNode->length == 0) {
            return true;
        }
        else {
            return false;   
        }
    } else  // invalid path
        return false;
}
int Wad::getSize(const string &path){
    if (this->isContent(path)) {
        list<string> parsed_path = parsePath(string(path));
        Node* foundNode = locateNode(parsed_path, this->root);

        return foundNode->length;
    } else
        return -1;
}
int Wad::getContents(const string &path, char *buffer, int length, int offset){
    if (this->isContent(string(path))) {
        Node* foundNode = locateNode(parsePath(string(path)), this->root);
        int fullOffset = foundNode->offset + offset;
        // if the content has less bytes than requested, 
        // update the length of the copy
        if (length > foundNode->length) {
            length = foundNode->length;
        }
        // copy `length` bytes into given buffer
        memcpy(buffer, &(this->data[fullOffset]), length);
        // return copied length
        return length;
    } else // is not content
        return -1;
}
int Wad::getDirectory(const string &path, vector<string> *directory){
    if (this->isDirectory(string(path))) {
        // get childrens' names
        Node* foundNode = locateNode(parsePath(string(path)), this->root);
        list<Node*> children = foundNode->children;

        // add the strings to `directory` in order
        int size = 0;
        for (Node* child : children){
            directory->push_back(child->name);
            size++;
        }
        return size;
    } else 
        return -1;
}