/**
Zespół 11

Norbert Bodziony
Sebastian Donchór
Paweł Gancarczyk
Konrad Krukar

użyto C++ Bitmap Library na licencji MIT autorstwa Arasha Partowa (patrz: bitmap_image.hpp)
użyto kodu algorytmy quicksort ze strony algorytm.org (algorytm zmodyfikowany na potrzeby programu)
**/

#include <string>
#include <iostream>
#include <stdio.h>
#include <fstream>
#include <math.h>
#include <algorithm>
#include <deque>
#include "bitmap_image.hpp"

using namespace std;

// GLOBALS
string path1;
string path2;
int type;
unsigned char* rawOutputArray; // tablica pikseli: w kazdym elemencie (co prawda 8-bitowym) zapisuje 5 bitow danego piksela rzedami
unsigned char* palette = new unsigned char[96]; // tu nalezy wpisac tablice kolorow, jezli tryb zapisu jest dedykowany
unsigned short* compressedOutputArray; // tablica liczb 16-bitowych (trojek ISN, patrz koncowka specyfikacji)
int width; // znane w czasie wywolania funkcji
int height; // znane w czasie wywolania funkcji
bitmap_image bitmapa;
bitmap_image robocza;
unsigned char* decompressedArray;
int decompressedOutputSize=0;
int compressedOutputSize=0;
int palette_counter = 0;

bool readArguments(int argc, char *argv[]) {
    if (argc != 4) return false;
    path1 = argv[1];
    path2 = argv[2];
    type = argv[3][0] - '0';
    return true;
}
bool readBMP() {
    bitmapa = bitmap_image(path1);
    if (!bitmapa) return false;
    width = bitmapa.width();
    height = bitmapa.height();
    return true;
}
bool readJKM() {
    ifstream bin;
    bin.open(path1.c_str(), ios::ate | ios::binary);
    int ilosc_szesnastek = (bin.tellg()-8)/2;
    bin.close();
    bin.open(path1.c_str(), ios::in | ios::binary);
    char cztery, siedemdziesiatszesc;
    bin.read(&cztery, 1);
    bin.read(&siedemdziesiatszesc, 1);
    if (cztery != 4 || siedemdziesiatszesc != 76) return false;
    bin.read(reinterpret_cast<char*>(&type), 2);
    bin.read(reinterpret_cast<char*>(&width), 4);
    if (type == 1) { // dedykowane, nalezy odczytac palete
        ilosc_szesnastek -= 48; // i jeszcze odjac jej rozmiar podzielony liczony w szesnastkach
        for (int i = 0; i < 96; i++) {
            bin.read(reinterpret_cast<char*>(&palette[i]), 1);
            //cout << (int)palette[i] << " ";
        }
    }
    compressedOutputArray = new unsigned short[ilosc_szesnastek];
    for(int i = 0; i < ilosc_szesnastek; i++) {
        bin.read(reinterpret_cast<char*>(&compressedOutputArray[i]), 2);
    }
    compressedOutputSize = ilosc_szesnastek;
    bin.close();
    return true;
}
bool createBMP() {
    height = decompressedOutputSize/width;
    bitmapa = bitmap_image(width, height);
}
bool save() {
    for (int i = 0; i < 96; i++) {
        //cout << (int)palette[i] << " ";
    }
    ofstream bin;
    bin.open(path2.c_str(), ios::out | ios::binary);
    bin << static_cast<unsigned char>(4);
    bin << static_cast<unsigned char>(76);
    bin.write(reinterpret_cast<const char*>(&type), 2);
    bin.write(reinterpret_cast<const char*>(&width), 4);
    if (type == 1) { // dedykowane, nalezy zapisac palete
        for (int i = 0; i < 96; i++) {
            bin << static_cast<unsigned char>(palette[i]);
            //cout << (int)static_cast<unsigned char>(palette[i]) << " ";
        }
    }
    for(int i = 0; i < compressedOutputSize; i++) {
        bin.write(reinterpret_cast<const char*>(&compressedOutputArray[i]), 2);
    }
    bin.close();
}
bool saveBMP() {
    bitmapa.save_image(path2);
}
bool writeForced() {
    rgb_t kolor;
    for(int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {
            bitmapa.get_pixel(x, y, kolor);
            rawOutputArray[y * width + x] = kolor.red >> 6;
            rawOutputArray[y * width + x] = rawOutputArray[y * width + x] << 2;
            rawOutputArray[y * width + x] += kolor.green >> 6;
            rawOutputArray[y * width + x] = rawOutputArray[y * width + x] << 1;
            rawOutputArray[y * width + x] += kolor.blue >> 7;
        }
    }
    return true;
}
bool writeGreyscale() {
    rgb_t kolor;
    for(int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {
            bitmapa.get_pixel(x, y, kolor);
            rawOutputArray[y * width + x] = 0.299*kolor.red + 0.587*kolor.green + 0.114*kolor.blue;
            rawOutputArray[y * width + x] = rawOutputArray[y * width + x] >> 3;
        }
    }
    return true;
}
bool readGreyscale() {
    for(int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {
            bitmapa.set_pixel(x, y, rawOutputArray[y * width + x]*8, rawOutputArray[y * width + x]*8, rawOutputArray[y * width + x]*8);
        }
    }
    return true;
}
bool readForced() {
    rgb_t kolor;
    for(int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {
            kolor.blue = rawOutputArray[y * width + x] << 7;
            kolor.blue >>= 7;
			kolor.blue*=255;
            kolor.green = rawOutputArray[y * width + x] >> 1;
            kolor.green <<= 6;
            kolor.green >>= 6;
			kolor.green*=(255/3);
            kolor.red = rawOutputArray[y * width + x] >> 3;
            kolor.red *=(255/3);

            bitmapa.set_pixel(x, y, kolor);
        }
    }
    return true;
}
int partition(int p, int r, int r_max, int r_red, int r_green) { // dzielimy tablice na dwie czesci, w pierwszej wszystkie liczby sa mniejsze badz rowne x, w drugiej wieksze lub rowne od x
    rgb_t x;
    robocza.get_pixel(p%width, p/width, x); // obieramy x
    int i = p, j = r; // i, j - indeksy w tabeli
    rgb_t w, u;
    while (true) // petla nieskonczona - wychodzimy z niej tylko przez return j
    {
        robocza.get_pixel(i%width, i/width, w);
        robocza.get_pixel(j%width, j/width, u);
        if(r_max == r_red) {
            while (u.red > x.red) { j--; robocza.get_pixel(j%width, j/width, u); }
            while (w.red < x.red) { i++; robocza.get_pixel(i%width, i/width, w); }
        }
        else if(r_max == r_green) {
            while (u.green > x.green) { j--; robocza.get_pixel(j%width, j/width, u); }
            while (w.green < x.green) { i++; robocza.get_pixel(i%width, i/width, w); }
        }
        else {
            while (u.blue > x.blue) { j--; robocza.get_pixel(j%width, j/width, u); }
            while (w.blue < x.blue) { i++; robocza.get_pixel(i%width, i/width, w); }
        }
        if (i < j) // zamieniamy miejscami gdy i < j
        {
            robocza.get_pixel(i%width, i/width, w);
            robocza.get_pixel(j%width, j/width, u);
            robocza.set_pixel(i%width, i/width, u);
            robocza.set_pixel(j%width, j/width, w);
            i++;
            j--;
        }
        else // gdy i >= j zwracamy j jako punkt podzialu tablicy
            return j;
    }
}
void quicksort(int p, int r, int r_max, int r_red, int r_green) { // sortowanie szybkie
    int q;
    if (p < r)
    {
        q = partition(p, r, r_max, r_red, r_green); // dzielimy tablice na dwie czesci; q oznacza punkt podzialu
        quicksort(p, q, r_max, r_red, r_green); // wywolujemy rekurencyjnie quicksort dla pierwszej czesci tablicy
        quicksort(q+1, r, r_max, r_red, r_green); // wywolujemy rekurencyjnie quicksort dla drugiej czesci tablicy
    }
}
void mediancut(int a, int b, int glebokosc) {
    if (glebokosc == 5) {
        rgb_t kolor;
        robocza.get_pixel((a+(b-a)/2)%width, (a+(b-a)/2)/width, kolor);
        palette[palette_counter*3] = kolor.red;
        palette[palette_counter*3+1] = kolor.green;
        palette[palette_counter*3+2] = kolor.blue;
        palette_counter++;
    }
    else {
        rgb_t kolor_min, kolor_max, kolor;
        robocza.get_pixel(0, 0, kolor_min);
        robocza.get_pixel(0, 0, kolor_max);
        for(int y = 0; y < height; y++) {
            for(int x = 1; x < width; x++) {
                bitmapa.get_pixel(0, 0, kolor);
                if(kolor.red > kolor_max.red) kolor_max.red = kolor.red;
                if(kolor.green > kolor_max.green) kolor_max.green = kolor.green;
                if(kolor.blue > kolor_max.blue) kolor_max.blue = kolor.blue;
                if(kolor.red < kolor_min.red) kolor_min.red = kolor.red;
                if(kolor.green < kolor_min.green) kolor_min.green = kolor.green;
                if(kolor.blue < kolor_min.blue) kolor_min.blue = kolor.blue;
            }
        }
        int r_red = kolor_max.red - kolor_min.red;
        int r_green = kolor_max.green - kolor_min.green;
        int r_blue = kolor_max.blue - kolor_min.blue;
        int r_max = max(r_red, max(r_green, r_blue));
        quicksort(a, b, r_max, r_red, r_green);
        mediancut(a, a+(b-a)/2, glebokosc+1);
        mediancut(a+(b-a)/2+1, b, glebokosc+1);
    }
}
bool matchColors() {
    rgb_t kolor;
    int najblizsza_odleglosc;
    int odleglosc;
    for(int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {
            bitmapa.get_pixel(x, y, kolor);
            najblizsza_odleglosc = 442;
            for(int i = 0; i < 32; i++) {
                odleglosc = sqrt(pow(palette[i*3]-kolor.red, 2) + pow(palette[i*3+1]-kolor.green, 2) + pow(palette[i*3+2]-kolor.blue, 2));
                if (odleglosc < najblizsza_odleglosc) {
                    rawOutputArray[y * width + x] = i;
                    najblizsza_odleglosc = odleglosc;
                }
            }
        }
    }
}
bool writeDedicated() {
    robocza = bitmap_image(path1);
    mediancut(0, width*height, 0);
    matchColors();
    return true;
}
bool readDedicated() {
    rgb_t kolor;
    for(int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {
            int indeks_w_palecie = rawOutputArray[y * width + x];
            kolor.red = palette[indeks_w_palecie*3];
            kolor.green = palette[indeks_w_palecie*3+1];
            kolor.blue = palette[indeks_w_palecie*3+2];
            bitmapa.set_pixel(x, y, kolor);
        }
    }
    return true;
}
bool decompress() {
    int size = compressedOutputSize;
    deque<char> outputDeque;
    for(int i=0;i<size;i++)
    {
        /**przetwarzanie ISN na skladowe**/
        unsigned short ISN=compressedOutputArray[i];
        unsigned char I=ISN>>11;
        unsigned char S=(ISN&0b0000011111100000)>>5;
        unsigned char N=ISN&0b0000000000011111;
        /**odkodowywanie I,S i N**/
        if(I==0)
        {
            outputDeque.push_back(N);
        }
        else
        {
            int endIndex=outputDeque.size();
            for(int j=0;j<I;j++)
            {
                outputDeque.push_back(outputDeque[endIndex-S]);
                endIndex++;
            }
            outputDeque.push_back(N);
        }
    }
    /**deque => array**/
    decompressedOutputSize=outputDeque.size();
    rawOutputArray=new unsigned char [decompressedOutputSize];
    for(int i=0;i<decompressedOutputSize;i++)
    {
        rawOutputArray[i]=outputDeque[i];
    }
    return true;
}
bool compress() {
    int rawOutSize = width*height;
    const int I_SIZE=32; //długość --0-31
    const int S_SIZE=63; //offset --0-63

    static deque<unsigned short> compressionOutput;
    static deque<unsigned char>searchBufferDeque;
    static deque<char>codingBufferDeque;

    int codingBufferIndex;
    int searchBufferJump;
    int maxSequenceLength=-1;
    int maxSequenceIndex=-1;

    /**inicjalizacja buforów**/
    searchBufferDeque.resize(S_SIZE);
    codingBufferDeque.resize(I_SIZE);
    for(int i=0;i<I_SIZE;i++)
    {
        codingBufferDeque[i]=rawOutputArray[i];
    }
    for(int i=0;i<searchBufferDeque.size();i++)
    {
        searchBufferDeque[i]=-1;
    }
    compressionOutput.push_back(0);
    compressionOutput.push_back(0);
    compressionOutput.push_back((int)codingBufferDeque[0]);
    codingBufferIndex=1;
    searchBufferJump=1;
    /**pętla główna**/
    int counter=0; //debug info
    while(codingBufferIndex<rawOutSize)
    {
        /**przesuniecie bufora slownikowego**/
        for(int i=0;i<searchBufferJump;i++)
        {
            searchBufferDeque.pop_front();
            searchBufferDeque.push_back(codingBufferDeque.at(i));
        }
        /**wczytanie nowego bufora kodowania**/
        for(int i=0;i<I_SIZE;i++)
        {
            if(i+codingBufferIndex<=rawOutSize-1)
            {
                codingBufferDeque.at(i)=rawOutputArray[i+codingBufferIndex];
            }
            else
            {
                codingBufferDeque.at(i)=-1;
            }
        }
        /**wyszukiwanie najdluzszego dopasowania**/
        for(int i=searchBufferDeque.size()-1;i>=0;i--)
        {
            int j=1;
            if(searchBufferDeque.at(i)!=-1)
            {
                if(searchBufferDeque.at(i)==codingBufferDeque.at(0))
                {
                    while(j<31&&i+j<64&&searchBufferDeque[i+j]==codingBufferDeque.at(j)&&codingBufferDeque.at(j+1)!=-1&&j<I_SIZE)
                    {
                        j++;
                    }
                    if(j>maxSequenceLength&&codingBufferDeque.at(j)!=-1)
                    {
                        maxSequenceIndex=i;
                        maxSequenceLength=j;
                    }
                }
            }
            else break;
        }
        /**zapisanie trojki ISN**/
        if(maxSequenceIndex==-1 )
        {
            if(codingBufferDeque.at(0)!=-1)
            {
                compressionOutput.push_back(0);
                compressionOutput.push_back(0);
                compressionOutput.push_back(codingBufferDeque.at(0));
            }
            codingBufferIndex+=1;
            searchBufferJump=1;
        }
        else
        {

            if((S_SIZE-maxSequenceIndex)<maxSequenceLength)maxSequenceLength=S_SIZE-maxSequenceIndex;
            compressionOutput.push_back(maxSequenceLength);
            compressionOutput.push_back(S_SIZE-maxSequenceIndex);
            compressionOutput.push_back(codingBufferDeque.at(maxSequenceLength));
            codingBufferIndex+=maxSequenceLength+1;
            searchBufferJump=maxSequenceLength+1;
        }
        /**debug info, zostaw zakomentowane na wszelki wypadek**/
     /**   cout<<"\nmax sequence index = "<<maxSequenceIndex<<" maxSequenceLength = "<<maxSequenceLength<<endl;
       cout<<"\nsearch buf "<<counter<<"\t";
        for(int i=0;i<S_SIZE;i++)
        {
            cout<<(int)searchBufferDeque[i]<<" ";
        }
       cout<<"\ncoding buf "<<counter<<"\t";
        for(int i=0;i<I_SIZE;i++)
        {
            cout<<(int)codingBufferDeque[i]<<" ";
        }
        cout<<"\ncompression out "<<counter<<"\t";
        int trojki=0;
        for(int i=0;i<compressionOutput.size();i++)
        {
            if(trojki>=3){cout<<" | ";trojki=0;}
            cout<<(int)compressionOutput[i]<<" ";
            trojki++;
        }
        cout<<"\n----------------------------------------------------------------\n";
        counter++;**/

        /**reset zmiennych**/
        maxSequenceIndex=-1;
        maxSequenceLength=-1;
    }
    /**laczenie trojek w jedna liczbe 16-bitowa**/
    compressedOutputArray=new unsigned short[compressionOutput.size()/3];
    int compressed_count=0;
    for(unsigned int i=0;i<compressionOutput.size()-1;i+=3)
    {
        if(i>=compressionOutput.size())break;
        unsigned char I=compressionOutput.at(i);
        unsigned char S=compressionOutput.at(i+1);
        unsigned char N=compressionOutput.at(i+2);
        unsigned short ISN=N|(S<<5)|(I<<11);
        compressedOutputArray[compressed_count]=ISN;
        compressed_count++;
    }
    compressedOutputSize=compressed_count;
    return true;
}

int main(int argc, char *argv[]) {
    if (!readArguments(argc, argv)) {
        cout << "Blad - wywolaj program z prawidlowymi argumentami:\njkm.exe sciezka1 sciezka2 typ";
    }

    path1="d.bmp";
    path2="dout.jkm";
    type=0;


    switch(type) {
    case 0: // narzucone
        readBMP();
        rawOutputArray = new unsigned char[width*height];
        writeForced();
        compress();
        save();
        break;

    case 1: // dedykowane
        readBMP();
        rawOutputArray = new unsigned char[width*height];
        writeDedicated();
        compress();
        save();
        break;

    case 2: // skala szaroœci
        readBMP();
        rawOutputArray = new unsigned char[width*height];
        writeGreyscale();
        compress();
        save();
        break;

    default: // odczyt do bmp
        if (!readJKM()) {
            cout << "Blad - nie mozna otworzyc pliku";
            return 2;
        }
        decompress();
        createBMP();
        switch(type) {
        case 0:
            readForced();
            break;
        case 1:
            readDedicated();
            break;
        case 2:
            readGreyscale();
        }
        saveBMP();
    }

    return 0;
}
