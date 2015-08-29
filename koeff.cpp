
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace std;

int main(){
    cout << fixed;
    cout << setprecision(3) ;
    
    int siz_in = 10;
    double ka, kb, kc;
    cout << "Y koeffs" << endl ;
    ka = 0.299;
    kb = 0.587;
    kc = 0.114;
    double bufa = ka /(1.);
    double bufb = kb /(1.);
    double bufc = kc /(1.);
    bufa *= 219./( (1 << siz_in) - 1);
    bufb *= 219./( (1 << siz_in) - 1);
    bufc *= 219./( (1 << siz_in) - 1);
    for (int i = 0; i < 11; ++i){
        cout << "shift " << i << endl;
        cout << bufa << " " << bufb <<" "<< bufc << endl;
        bufa *= 2.;
        bufb *= 2.;
        bufc *= 2.;
    }
    cout<< "******************************************\n";
    
    cout << "Cb koeffs" << endl ;
    ka = -0.299;
    kb = -0.587;
    kc = -0.886;
    bufa = ka /(1.772);
    bufb = kb /(1.772);
    bufc = kc /(1.772);
    bufa *= 224./( (1 << siz_in) - 1);
    bufb *= 224./( (1 << siz_in) - 1);
    bufc *= 224./( (1 << siz_in) - 1);
    for (int i = 0; i < 11; ++i){
        cout << " shift " << i << endl;
        cout << bufa << " " << bufb <<" "<< bufc << endl;
        bufa *= 2.;
        bufb *= 2.;
        bufc *= 2.;
    }
    cout<< "******************************************\n";
    
    cout << "Cr koeffs" << endl ;
    ka = 0.701;
    kb = -0.587;
    kc = -0.114;
    bufa = ka /(1.402);
    bufb = kb /(1.402);
    bufc = kc /(1.402);
    bufa *= 224./( (1 << siz_in) - 1);
    bufb *= 224./( (1 << siz_in) - 1);
    bufc *= 224./( (1 << siz_in) - 1);
    for (int i = 0; i < 11; ++i){
        cout << " shift " << i << endl;
        cout << bufa << " " << bufb <<" "<< bufc << endl;
        bufa *= 2.;
        bufb *= 2.;
        bufc *= 2.;
    }
    return 0;
}
