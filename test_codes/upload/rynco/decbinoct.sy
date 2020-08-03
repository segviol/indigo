int main(){
    int n = getint();
    int nb = n;
    int no = n;
    int nh = n;
    int binlen = 0;
    int octlen = 0;
    int hexlen = 0;
    int bin[32]={};
    int oct[32]={};
    int hex[32]={};
    while(nb > 0){
        int bd = nb % 2;
        bin[binlen] = bd;
        binlen = binlen + 1;
        nb = nb / 2;
    }
    while(binlen > 0){
        binlen = binlen - 1;
        putint(bin[binlen]);
    }
    putch(32);
    while(no > 0){
        int od = no % 8;
        oct[octlen] = od;
        octlen = octlen + 1;
        no = no / 8;
    }
    while(octlen > 0){
        octlen = octlen - 1;
        putint(oct[octlen]);
    }
    putch(32);
    while(nh > 0){
        int hd = nh % 16;
        hex[hexlen] = hd;
        hexlen = hexlen + 1;
        nh = nh / 16;
    }
    while(hexlen > 0){
        hexlen = hexlen - 1;
        int hd = hex[hexlen];
        if(hd < 10){
            putch(hd+48);
        }else{
            putch(hd-10+65);
        }
    }
    return 0;
}
