#include <DigiPotX9Cxxx.h>
#include <VirtualWire.h>

int potValor=0;//Simulacion

const uint8_t SEGUNDOS_TIMEOUT_DERIVANDO=20; //Segundos que esperamos hasta parar la derivación

const uint8_t LIMITE_SUPERIOR=50; //Objetivo, tener un consumo constante de 50w
const uint8_t LIMITE_INFERIOR=0; //Objetivo, nunca por debajo de cero

float FACTOR_CONVERSOR_WATIOS=3.33;//Calculado del consumo tope del calentador/Resistencia del potenciómetro

const uint8_t VALOR_R_VARIABLE=99; //Objetivo, nunca por debajo de cero
const uint8_t VALOR_R_FIJA=100; //Valor de 3 resistencias fijas de 100k
const int RTOTAL=VALOR_R_FIJA*4 + VALOR_R_VARIABLE;

const uint8_t PIN_DERIVANDO=A0; //Objetivo, nunca por debajo de cero
const uint8_t PIN_R1=A0; //Pin R1
const uint8_t PIN_R2=A1; //Pin R2
const uint8_t PIN_R3=A2; //Pin R3
const uint8_t PIN_R4=A3; //Pin R4

boolean estaDerivando = false;
boolean r1 = true;
boolean r2 = true;
boolean r3 = true;
boolean r4 = true;
unsigned long tIniDerivandoAlMinimo=0; //Tiempo que servirá para almacenar la última vez que se superó el umbral de 0, para activar el timeout de la derivacion

long milisLecturaAnterior=0;

// the setup routine runs once when you press reset:
DigiPot pot(2,3,4);

void setup() {

  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);

  Serial.println("www.calentadorsolarpasoapaso.blogspot.com");
  
  pinMode(PIN_DERIVANDO, OUTPUT); 
  pinMode(PIN_R1, OUTPUT); 
  pinMode(PIN_R2, OUTPUT); 
  pinMode(PIN_R3, OUTPUT); 
  pinMode(PIN_R4, OUTPUT); 

  activarResistenciaFija1();
  activarResistenciaFija2();
  activarResistenciaFija3();
  activarResistenciaFija4();
  
  //Resistencia al mÃ¡ximo
  setValorActualRVariable(VALOR_R_VARIABLE);

  //Cerramos la salida de 220. Por defecto cerrada, pero por si acaso
  desactivarDerivacion();

  setupRadioFrecuencia();  
}


// the loop routine runs over and over again forever:
void loop() {
  int watts=leerValorRadioFrecuencia();
  Serial.print("Watios leidos");
  Serial.println(watts);
  ajustarSalida( watts);  

  /*
  ajustarSalida( 20);  
  ajustarSalida( 40);  
  ajustarSalida( 50);  
  ajustarSalida( 0);  
  ajustarSalida( -100);  
  ajustarSalida( -200);  
  ajustarSalida( -230);  
  ajustarSalida( -250);  
  ajustarSalida( -850);  
  ajustarSalida( -950);  
  ajustarSalida( -1250);  
  ajustarSalida( -1350);  
  ajustarSalida( 20);  
  ajustarSalida( 200);  
  ajustarSalida( 200);  
  ajustarSalida( 1500);  
  ajustarSalida( 1500);  
  ajustarSalida( 1500);  
  ajustarSalida( 1500);  
  ajustarSalida( 1500);  
  ajustarSalida( 200);  
  ajustarSalida( 100);  
  ajustarSalida( 200);  
  ajustarSalida( 300);  
  ajustarSalida( 150);  
  */
}

/*
Tenemos una placa de relays de 4 elementos, lo que nos permite jugar con ellos para ajustar al máximo la salida
El chip con resistencia variable, ajustará tramos de 0-99k y en 3 puertos meteremos 1 resistencia de 100k para activar o desactivarlas
definiendo así un rango variable de 0-399kw. Notese que el potenciómetro tiene de 0-500, aunque los primeros 100 no sirven para mucho
*/

int ajustarSalida( int watios){
  int incrementoEstimadoR=0;
  uint8_t valorIncremento=0;
  uint8_t resistenciaActual=getValorActualRVariable();
  //puede pasar que tenga que decrementar mÃ¡s de una resistencia de 100k
  int rTotalActual=getValorRFija() + resistenciaActual;

  if(watios<=LIMITE_INFERIOR){
        if(!estaDerivando){
          activarDerivacion(); //Se activa la derivación al mínimo de derivación
        }
    }

  if(estaDerivando){
  
      incrementoEstimadoR=calcularIncrementoEstimadoR(watios,rTotalActual,LIMITE_SUPERIOR,LIMITE_INFERIOR);
      modificarResistenciaPotenciometro(incrementoEstimadoR,rTotalActual);
  
    if(r1 && r2 && r3 && r4 && (resistenciaActual==VALOR_R_VARIABLE) && estaDerivando){
  
      //Me guardo el tiempo que lleva el sistema al mínimo de potencia derivada
  
      if(tIniDerivandoAlMinimo==0){
        tIniDerivandoAlMinimo=millis();
      }
  
      //Si derivando > 5 minutos, paramos la derivación
      if(timeoutDesactivarDerivacion()) desactivarDerivacion();
    }
    else{
      tIniDerivandoAlMinimo=0;
    }
  }


  imprimirDatos(watios,incrementoEstimadoR);  

  //Delay para ver los valores en el emulador
  delay(1000);

  return incrementoEstimadoR; 
}

int getResistenciaActual(){
  return getValorRFija() + getValorActualRVariable();
}

float calcularWatiosDerivando(){
  float wDerivando=(RTOTAL-getResistenciaActual())*FACTOR_CONVERSOR_WATIOS;
  return wDerivando;
}


void imprimirDatos(int watios,int incrementoEstimadoR){
    float wDerivando=calcularWatiosDerivando();
   int rTotalActual=getResistenciaActual();
/*
  Serial.print("Wat:\t");
  Serial.print(watios);
  Serial.print("\twDeriv:\t");
  Serial.print(wDerivando);
  Serial.print("\tRT:\t");
  Serial.print(rTotalActual);
  Serial.print("\tInc:");
  Serial.print(incrementoEstimadoR);
  Serial.print("\tpot\t");
  Serial.print(getValorActualRVariable());
  Serial.print(" D: ");
  Serial.print(estaDerivando);
  Serial.print(" ");
  Serial.println(" ");
  */
}

int modificarResistenciaPotenciometro(int incrementoEstimadoR,int rTotalActual){
  //Si la resistencia no varía, salimos

  if (incrementoEstimadoR==0) return 0;
  
  int rTotalEstimada=rTotalActual+incrementoEstimadoR;
  
  rTotalEstimada=constrain(rTotalEstimada, 0, RTOTAL);  

  int nRActivas=rTotalEstimada/VALOR_R_FIJA;
  int rVariable=rTotalEstimada%VALOR_R_FIJA;
  
  //Variamos resistencias fijas
  switch (nRActivas) {
    case 4:
      if(!r4) activarResistenciaFija4();
      if(!r3) activarResistenciaFija3();
      if(!r2) activarResistenciaFija2();
      if(!r1) activarResistenciaFija1();
      break;
    case 3:
      if(r4)   desactivarResistenciaFija4();
      if(!r3)    activarResistenciaFija3();
      if(!r2)    activarResistenciaFija2();
      if(!r1)    activarResistenciaFija1();
      break;
    case 2:
      if(r4) desactivarResistenciaFija4();
      if(r3) desactivarResistenciaFija3();
      if(!r2)   activarResistenciaFija2();
      if(!r1)   activarResistenciaFija1();
      break;
    case 1:
      if(r4) desactivarResistenciaFija4();
      if(r3) desactivarResistenciaFija3();
      if(r2) desactivarResistenciaFija2();
      if(!r1)   activarResistenciaFija1();
      break;
    default: 
      desactivarResistenciaFija4();
      desactivarResistenciaFija3();
      desactivarResistenciaFija2();
      desactivarResistenciaFija1();
      break;
  }

  //Variamos resistencia variable
  setValorActualRVariable(rVariable);
  return rVariable;
  }


/*
Funcion que estima el incremento o decremento que deberá tener la resistencia para aproximarnos a 0 sin sobrepasar

los límites (LIMITE_INFERIOR,LIMITE_SUPERIOR).

En un futuro, debería almacenar un histórico y basarse en los últimos x valores para realizar la estimación. ASí, las subidas abruptas serían 

estimadas  por tanto no inyectadas a red, 
El valor actual de rTotal no sirve de mucho, pero es posible que tenga que usar exponenciales para realizar la estimación

*/
  /*Calculamos la logica que nos diga cuanto incrementar
    Convertir de watios a resistencia(R)
    Para calcular la cantidad de elementos de resistencia a subir o bajar, tengo que saber
    la carga de la potencia (vamos, lo que gasta el cacharro que le enchufemos)
    No es lo mismo, si le metemos 500W que si le metemos 2000, el valor de R cambiaría

    Otra forma, sería aprendiendo cuanto sube con cada incremento simple, pero necesitaríamos memoria flash

    
  */

int calcularIncrementoEstimadoR(int watios,int rTotal,uint8_t LIMITE_SUPERIOR,uint8_t LIMITE_INFERIOR){
    int r=0;
    //Solo modificaremos la resistencia si estamos fuera de rangos. 
    //Estos valores tendrían que autocalcularse en función del calentador enchufado al potenciometro. Revisar
    if(watios>LIMITE_SUPERIOR || watios<LIMITE_INFERIOR){
//      r=map(watios, -1000, 1000, -RTOTAL, RTOTAL);
      r=watios/FACTOR_CONVERSOR_WATIOS;

    }
    //La resistencia tiene un rango de 0-400, nunca debe sobrepasarlo
    r=constrain(r, -(RTOTAL), (RTOTAL));  
   
    return r;
}

int getValorRFija(){
  int r=0;
  if(r1) r+=VALOR_R_FIJA; //R1=100k;
  if(r2) r+=VALOR_R_FIJA; //R2=100k;
  if(r3) r+=VALOR_R_FIJA; //R3=100k;
  if(r4) r+=VALOR_R_FIJA; //R4=100k;
  return r;
}
void desactivarResistenciaFija4(){
  r4=false;
  digitalWrite(PIN_R4,LOW);
}  

void desactivarResistenciaFija3(){
  r3=false;
  digitalWrite(PIN_R3,LOW);
}  
  
void desactivarResistenciaFija2(){
  r2=false;
  digitalWrite(PIN_R2,LOW);
}  
void desactivarResistenciaFija1(){
  r1=false;
  digitalWrite(PIN_R1,LOW);
}  

void activarResistenciaFija4(){
  r4=true;
  digitalWrite(PIN_R4,HIGH);
}  

void activarResistenciaFija3(){
  r3=true;
  digitalWrite(PIN_R3,HIGH);
}  
  
void activarResistenciaFija2(){
  r2=true;
  digitalWrite(PIN_R2,HIGH);
}  
void activarResistenciaFija1(){
  r1=true;
  digitalWrite(PIN_R1,HIGH);
}  

void activarDerivacion(){
  estaDerivando=true;
  digitalWrite(PIN_DERIVANDO,HIGH);

  Serial.println("ACTIVO DERIVACION");

}  

void desactivarDerivacion(){
  estaDerivando=false;
  digitalWrite(PIN_DERIVANDO,LOW);
  //Resistencia variable a 100
  setValorActualRVariable(VALOR_R_VARIABLE);
  
  //no debería pasar, pero por si acaso, inicializamos Rs

  activarResistenciaFija1();
  activarResistenciaFija2();
  activarResistenciaFija3();
  activarResistenciaFija4();
  
  tIniDerivandoAlMinimo=0;
  
    Serial.println("DETENGO DERIVACION");

}  

boolean timeoutDesactivarDerivacion(){
  return (millis()-tIniDerivandoAlMinimo)>(SEGUNDOS_TIMEOUT_DERIVANDO*1000);
}

int getValorActualRVariable(){
  //return potValor;
  return pot.get();
}

void setValorActualRVariable(int rVariable){
//  potValor=rVariable; 
  pot.set(rVariable);
}

void setupRadioFrecuencia(){
    Serial.println("Incializando RF");

    // Initialise the IO and ISR
    vw_set_ptt_inverted(true); // Required for DR3100
    vw_setup(2000);	 // Bits per sec

    vw_rx_start();       // Start the receiver PLL running
}

int leerValorRadioFrecuencia(){
    uint8_t buf[VW_MAX_MESSAGE_LEN];
    uint8_t buflen = VW_MAX_MESSAGE_LEN;

    if (vw_get_message(buf, &buflen)) // Non-blocking
    {
	int i;
        long ms=millis()-milisLecturaAnterior;
              
        milisLecturaAnterior=millis();
        //digitalWrite(13, true); // Flash a light to show received good message
	// Message with a good checksum received, dump it.
	Serial.print("Leido: ");
	Serial.print(" ms:");
	Serial.print(ms);
	Serial.print(" Watts:");
	String valor="";
	for (i = 0; i < buflen; i++)
	{
            char caracter=(char)buf[i];
              valor+=caracter;
	}
        
        String watts=valor.substring(0,valor.indexOf(' ')-1);
        
        Serial.print("Watios ");
	Serial.println(watts);
        return watts.toInt();
        //digitalWrite(13, false);
    }
}
