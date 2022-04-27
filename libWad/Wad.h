#pragma once
#include <string>
#include <list>
#include <vector>
#include <fstream>

using namespace std;

struct Node {
    int length;
    int offset;
    string name;
    list<Node*> children;

    Node(int length, int offset, string name, list<Node*> children = list<Node*>());
};

class Wad {
    private:
        string magic;
        int numDesc;
        int descOffset;
        char* data;
        Node* root;
    public:
        Wad(string magic, int numDesc, int descOffset, Node* root, char* file);
        static Wad* loadWad(const string &path);
        string getMagic();
        bool isContent(const string &path);
        bool isDirectory(const string &path);
        int getSize(const string &path);
        int getContents(const string &path, char *buffer, int length, int offset = 0);
        int getDirectory(const string &path, vector<string> *directory);
};