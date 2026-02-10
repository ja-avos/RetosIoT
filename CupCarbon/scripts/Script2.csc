set ant 999

atget id id
getpos2 lonSen latSen

loop
wait 100
read mens
rdata mens tipo valor

if((tipo=="hola") && (ant == 999))
   set ant valor
   data mens tipo id
   send mens * valor
end

if(tipo=="alerta")
   send mens ant
end
delay 1000

areadsensor tempSen
rdata tempSen SensTipo idSens temp

if( temp>30)
   data mens "alerta" lonSen latSen
   send mens ant
end