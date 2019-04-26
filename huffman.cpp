#include<iostream>
#include<string>
#include<fstream>
#include<cstdlib>
#include<algorithm>
#include<vector>
#include<climits>
#include <new>


#define sizeOfFreq 256

using namespace std;

struct Character {
	unsigned ascii;
	unsigned long long frequency;

	Character(unsigned ascii = UINT_MAX, unsigned long long frequency = 0) {
		this->ascii = ascii;
		this->frequency = frequency;
	}
};
struct Node {
	Node* left;
	Character character;
	Node* right;
};
// Main functions
void getFreq(unsigned long long freq[sizeOfFreq], const string& fileName);
void huffman(string code[256], const unsigned long long freq[256]);
void read_and_write_to_compressed_file(const string& originalFileName, const string& compressedFileName, string code[256]);

// Helper functions
Node* getNode(unsigned ascii = UINT_MAX, unsigned long long frequency = 0);
Node* getLeastFrequencyNode(vector<Node*>&);

void nodeSort(vector<Node*>& v);
void inOrderTransversal(Node * root, string code[sizeOfFreq], string& binCodeLeft);

void die(const string& err);


int main() {
	unsigned long long freq[sizeOfFreq] = { 0 };
	string code[sizeOfFreq] = { "" };

	string input_fileName, output_fileName;
	
	cout << "Please enter a input fileName ";
	getline(cin, input_fileName);
	// Gets the input filename

	getFreq(freq, input_fileName.c_str());
	huffman(code, freq);
	// Generates the huffman code and stores it in code array

	cout << "Please enter your output filename ";
	getline(cin, output_fileName);
	// Gets the output fileName

	read_and_write_to_compressed_file(input_fileName.c_str(), output_fileName.c_str(), code);
	// Reads and write to the compressed file

	return 0;
}


void getFreq(unsigned long long freq[sizeOfFreq], const string& fileName) {

	fill(freq, freq + sizeOfFreq, 0);

	ifstream infile(fileName, ios::binary);
	if (!infile)
		die("Wasn't able to open the " + fileName + " in binary mode");

	for (int c = infile.get(); !infile.eof(); c = infile.get())
		if (c == EOF)
			die("Error reading the file. Couldn't read the entire file");
		else 
			freq[c]++;
	
	infile.close();
}
void huffman(string code[256], const unsigned long long freq[256]) {
	fill(code, code + sizeOfFreq, "");
	// Fill the code array with nothing string 

	vector<Node *> v;
	for (int i = 0; i < sizeOfFreq; i++)
		if (freq[i] > 0)
			v.push_back(getNode(i, freq[i]));
	// Create a vector of Nodes whose character have frequency greater than 0

	if (v.size() == 0)
		return;
	// So if we nothing in the file there is no huffman code to be made
	nodeSort(v);
	// Sort the vector array

	if (v.size() == 1) {
		code[v[0]->character.ascii] = '0';
		cout << "Ascii = " << v[0]->character.ascii << " Frequency = " << v[0]->character.frequency << " Code = "  << 
			code[v[0]->character.ascii] << endl;
		return;
	}
	// If you only have one character in the arrae
	else {
		while (v.size() != 1) {

			Node* left = getLeastFrequencyNode(v);
			Node * right = getLeastFrequencyNode(v);
			// Get 2 nodes with least frequency

			Node * parent = getNode(-1, left->character.frequency + right->character.frequency);
			// Make a parent whose frequency has to be the sum of the 2 frequencies of the characters with the least frequency

			parent->left = left;
			parent->right = right;
			// Connect the parent Node to the children

			v.push_back(parent);
			// Push it on the vector
		}
		Node * root = v.back();
		v.pop_back();

		string binCode;
		inOrderTransversal(root, code, binCode);

	}
	
}
void read_and_write_to_compressed_file(const string& originalFileName, const string& compressedFileName, string code[256]) {

	ifstream infile(originalFileName, ios::binary | ios::ate);
	// ifstream variable opens the file in binary mode and sets the get pointer to the end of the file

	if (!infile) die("Can't reopen the input file"); 
	// Checking if we opened the file correctly

	unsigned long long lengthOfFile = infile.tellg();
	infile.seekg(0);
	// Getting the length of the input file

	ofstream outfile(compressedFileName, ios::binary | ios::out);
	if (!outfile) die("Error opening the huffman code file");
	// Checking if the huffman file was opened or not
	
	char byte = 0;
	for (int i = 63; i >= 0; i--) {

		char bit = (lengthOfFile >> i) & 1;
		byte |= bit; // Puts the bit at the end

		if (i % 8 == 0) {
			outfile.put(byte);
			byte = 0;
		} // When we have added the 8th bit we put it in the output file

		else byte <<= 1; // Moves the bit to the left

	} // Writing the length of the input file in the huffman file
	
	string keeper;

	for (char c = infile.get(); !infile.eof(); c = infile.get()) {
		if (c == EOF)
			die("Error re-reading the file");
		else {
			keeper.append(code[c]);
			while (keeper.length() >= 8) {
				char byte = 0;
				for (int i = 0; i < 8; i++) {
					byte <<= 1;
					if (keeper[i] == '1')
						byte |= 1;
				}
				outfile.put(byte);
				// Write 1 byte at time until the length of keeper is less than 8
				keeper = keeper.substr(8); // Gets the string from index 8 till the end
			}
		}
	}
	
	infile.close(); // We are done with the input file now

	// Writes the huffman code but maybe the last byte isn't complete yet so we gotta complete it and write it
	if (keeper.length() != 0) {
		keeper.append(8 - keeper.length(), '0'); // Puts appropriate amounts of 0's at the end to complete the byte
		for (int i = 0; i < 8; i++) {
			byte <<= 1;
			if (keeper[i] == '1')
				byte |= 1;
		}
		outfile.put(byte); // Writes that last byte
	}
	// We are done now and we can close the outputfile
	outfile.close();

	infile.open(compressedFileName, ios::binary); 
	// Open the newly written huffman file

	// First we need to read the first 8 bytes which gives the number of chars that we need to decode from the file
	unsigned long long length_of_compressed_file = 0;
	for (int j = 0; j < 8; j++) {
		length_of_compressed_file |= infile.get();
		if(j != 7)
			length_of_compressed_file <<= 8;
	}
	
	// Decoding huffman code 

	unsigned long long total_chars = 0;
	string bits_left; // Contains the bits that haven't been decoded

	while (total_chars != length_of_compressed_file) {
		char byte = infile.get(); // Get the byte

		string bits;
		for (int i = 8; i > 0; i--)
			bits += ( ( byte >> ( i-1 ) ) & 1 ) + '0'; // Convert the byte into individual bit string
		
		for (int i = 0; i < 8 && total_chars != length_of_compressed_file; i++) {
			bits_left += bits[i]; // Put individual bit string into undeciphered bits_left variable 
								 // and check if it matches any other string in the code array
			for (int j = 0; j < sizeOfFreq; j++)
				if (bits_left == code[j]) { // If we have a match then output the ascii char to the console
					cout << (char)j;
					bits_left = ""; // Change the bits_left to empty string since we deciphered it
					total_chars++; // Increment the amounts of character we have deciphered
					break; // Break since we found it already
				}
		}
	}
	infile.close();
}
Node* getNode(unsigned ascii, unsigned long long frequency){
	Node * ret = nullptr;
	try {
		ret = new Node;
		ret->character = Character(ascii, frequency);
		ret->left = ret->right = nullptr;
	}
	catch (bad_alloc&) {
		die("Allocation failure");
	}
	return ret;
}
void nodeSort(vector<Node*>& v) {
	for (unsigned i = 0; i < v.size(); i++)
		for (unsigned j = i + 1; j < v.size(); j++)
			if (v[i]->character.frequency < v[j]->character.frequency) {
				Node * temp = v[i];
				v[i] = v[j];
				v[j] = temp;
			}
}
Node* getLeastFrequencyNode(vector<Node*>& v) {
	unsigned minIndex = 0;
	
	for (unsigned i = 1; i < v.size(); i++)
		if (v[minIndex]->character.frequency > v[i]->character.frequency)
			minIndex = i;

	Node * ret = v[minIndex];

	v.erase(v.begin() + minIndex);

	return ret;
}
void inOrderTransversal(Node* root, string code[sizeOfFreq], string& binCode) {
	if (root != nullptr)
		if (root->left) {
			binCode += "0";	// when we go left we add 0
			inOrderTransversal(root->left, code, binCode);

			binCode += "1"; // When we go right we add 1
			inOrderTransversal(root->right, code, binCode);
			binCode = binCode.substr(0, binCode.length() - 1); 

			// This removes the latest 0 that was added by latest inOrderTransversal
		}
		else {
			code[root->character.ascii] = binCode;
			binCode = binCode.substr(0, binCode.length() - 1); 

			// This gets rid of the last character of binCode
		}
}

void die(const string& err) {
	cout << "Fatal Error: " << err << endl;
	exit(EXIT_FAILURE);
}
