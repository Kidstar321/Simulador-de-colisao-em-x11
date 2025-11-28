#define _POSIX_C_SOURCE 199309L
#include <X11/Xlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>

#define LARGURA_W 1280
#define ALTURA_W 720
#define FPS 60
#define DT 1.0/FPS

typedef struct 
{
    int x,y;
    float vx,vy;
    int r;
    char cor[8];
}Bola;

double atualSegundos() //me da os segundos na precisão de 1e9, baugulho é brabo!
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

void desenharBola(Display *d,Window w,GC gc,Bola bola) //auto-explicativo
{
    XColor color;
    Colormap cmap = DefaultColormap(d, DefaultScreen(d)); 

    XParseColor(d, cmap,bola.cor, &color);	
    XAllocColor(d, cmap, &color);
    XSetForeground(d, gc, color.pixel); 

    for(int i=-bola.r;i<=bola.r;i++)
    {
        for(int j=-bola.r;j<=bola.r;j++)
        {
            if(i*i+j*j<=bola.r*bola.r) XDrawPoint(d, w, gc, bola.x+i, bola.y+j);
        }
    }
}

void limpaTela(Display *d,Window w,GC gc,char cor[8]) //também auto-explicativo
{
    XColor color;
    Colormap cmap = DefaultColormap(d, DefaultScreen(d)); 

    XParseColor(d, cmap,cor, &color);	
    XAllocColor(d, cmap, &color);
    XSetForeground(d, gc, color.pixel); 

    XFillRectangle(d, w, gc, 0, 0, LARGURA_W, ALTURA_W);
}

bool algumOverlap(Bola emanalise,int nbola, Bola bolas[],int n,int *qualbola,double *overlap) //checa se essa bola ta dentro de alguma outra
{
    int dquadrado;

    for(int i=0;i<n;i++)
    {
        if(i==nbola) continue;
        dquadrado=(emanalise.x-bolas[i].x)*(emanalise.x-bolas[i].x)+(emanalise.y-bolas[i].y)*(emanalise.y-bolas[i].y);
        if(dquadrado<(emanalise.r+bolas[i].r)*(emanalise.r+bolas[i].r)) 
        {
            *qualbola=i;
            *overlap=emanalise.r+bolas[i].r-sqrt(dquadrado);
            return 1;
        }
    }
    return 0;
}

int main()
{
    Display *d;
    Window w;
    XEvent e;
    int s;
    
    d = XOpenDisplay(NULL); //pointer d funciona como um link entre o codigo e o meu display
    if (d == NULL) {
        return 1;
    }

    s = DefaultScreen(d); //usa o monitor principal	

    w = XCreateSimpleWindow(d, RootWindow(d, s), 10, 10, LARGURA_W, ALTURA_W, 1, //cria a janela em uma certa posição, tamanho e grossura da borda
                            BlackPixel(d, s), WhitePixel(d, s));

    XSelectInput(d, w, ExposureMask | KeyPressMask); //especifica que tipos de info eu vou querer trocar com a janela
    XMapWindow(d, w); //faz a janela ser visivel

    Atom wmDelete = XInternAtom(d, "WM_DELETE_WINDOW", False); //recebe a info se o botao de fechar janela foi clicado
    XSetWMProtocols(d, w, &wmDelete, 1);

    GC gc = XCreateGC(d, w, 0, NULL); //Cria o pincel que eu vou usar pra renderizar os pixels

    Pixmap buffer = XCreatePixmap(d, w, LARGURA_W, ALTURA_W, DefaultDepth(d, s)); //area que eu vou usar pra fazer o buffer da textura

    srand(time(NULL));
    int cronometro=0;
    int n_criadas=0;
    bool temoverlap;
    int i,j;

    Bola* bolas;
    bolas=(Bola*)malloc(10*sizeof(Bola));

    int qualcolis; //identificador de qual bola colidiu com qual
    double anterior_t=0.0,atual_t;
    bool rodando=1;

    //var aux para as colisões
    float van,vbn,van2,vbn2;
    float vat,vbt;
    int ma,mb;
    double sin,cos;
    float vaux;
    double overlap,dist,corr;
    int aux;
    int dx,dy;


    while(rodando) 
    {
        while(XPending(d))
        {
            XNextEvent(d, &e);
            if (e.type == ClientMessage) 
            {
                if ((Atom)e.xclient.data.l[0] == wmDelete)
                    rodando = 0;
            }
        }

        atual_t=atualSegundos();
        if(atual_t-anterior_t>=DT) //loop onde a magia acontece baby, roda a cada 1 frame
        {
            anterior_t=atual_t;
            cronometro++;
            limpaTela(d,buffer,gc,"#000000");
            
            if(n_criadas<10&&cronometro>=60) //spawn de nova bola
            {
                cronometro=0;

                bolas[n_criadas].r=25+rand()%76; //r entre 25 e 100

                temoverlap=1;
                while(temoverlap)
                {
                bolas[n_criadas].x=rand()%(LARGURA_W+1);
                bolas[n_criadas].y=rand()%(ALTURA_W+1);

                temoverlap=algumOverlap(bolas[n_criadas],n_criadas,bolas,n_criadas,&qualcolis,&overlap);
                }

                bolas[n_criadas].vx=-200+(double)rand()/RAND_MAX*400; //v entre -200 e 200
                bolas[n_criadas].vy=-200+(double)rand()/RAND_MAX*400;

                sprintf(bolas[n_criadas].cor,"#%06X",rand()%0x1000000); //cor aleatoria

                n_criadas++;
            }

            for(i=0;i<n_criadas;i++) //atualização das bolas
            {   
                //colisões com a parede
                if(bolas[i].y<bolas[i].r) 
                {
                    bolas[i].vy*=-1;
                    bolas[i].y=bolas[i].r+1;
                }
                else if(bolas[i].y>(ALTURA_W-bolas[i].r))
                {
                    bolas[i].vy*=-1;
                    bolas[i].y=ALTURA_W-bolas[i].r-1;
                }
                if(bolas[i].x<bolas[i].r) 
                {
                    bolas[i].vx*=-1;
                    bolas[i].x=bolas[i].r+1;

                }
                else if(bolas[i].x>(LARGURA_W-bolas[i].r))
                {
                    bolas[i].vx*=-1;
                    bolas[i].x=LARGURA_W-bolas[i].r-1;  
                }
                bolas[i].x+=bolas[i].vx*DT;
                bolas[i].y+=bolas[i].vy*DT;

                if(algumOverlap(bolas[i],i,bolas,n_criadas,&qualcolis,&overlap)) //deve ser feito colisao, por enquanto perfeitamente elastica
                {
                    dx = bolas[i].x - bolas[qualcolis].x;
                    dy = bolas[i].y - bolas[qualcolis].y;

                    dist = sqrt(dx*dx + dy*dy);
                    if(dist < 0.0000001) dist = 0.0000001;

                    //seno e cosseno entre eixo xy e eixo das colisoes
                    cos = dx / dist;
                    sin = dy / dist;

                    // velocidades normais
                    van = bolas[i].vx * cos + bolas[i].vy * sin;
                    vbn = bolas[qualcolis].vx * cos + bolas[qualcolis].vy * sin;

                    // velocidades tangencias
                    vat = -bolas[i].vx * sin + bolas[i].vy * cos;
                    vbt = -bolas[qualcolis].vx * sin + bolas[qualcolis].vy * cos;

                    // massas considerando que a bolas vivem num mundo 2d e são feitas do mesmo material
                    ma = bolas[i].r * bolas[i].r;
                    mb = bolas[qualcolis].r * bolas[qualcolis].r;

                    // colsaio elastica
                    vaux = van;
                    van = (van*(ma-mb) + 2*mb*vbn) / (ma+mb);
                    vbn = (vbn*(mb-ma) + 2*ma*vaux) / (ma+mb);

                    //transforma de volta para o eixo xy
                    bolas[i].vx = van * cos - vat * sin;
                    bolas[i].vy = van * sin + vat * cos;

                    bolas[qualcolis].vx = vbn * cos - vbt * sin;
                    bolas[qualcolis].vy = vbn * sin + vbt * cos;

                    //correção de posição

                    corr = overlap / 2.0;

                    bolas[i].x += cos * corr;
                    bolas[i].y += sin * corr;

                    bolas[qualcolis].x -= cos * corr;
                    bolas[qualcolis].y -= sin * corr;
                } 

                    desenharBola(d,buffer,gc,bolas[i]);
            }

            XCopyArea(d, buffer, w, gc, 0, 0, LARGURA_W, ALTURA_W, 0, 0);
            XFlush(d);
        }

        usleep(1000);
    }


}