set ant 999
set ite 0
battery set 100

atget id id
getpos2 lonSen latSen

loop

delay 100

inc ite
print ite

read mens
rdata mens tipo valor

if (ite >= 1000)
	stop
end

if (tipo=="stop")
	data mens "stop"
	send mens * valor
	cprint "Para sensor: " id
	wait 1000
	stop
end

battery bat
if(bat<5)
	data mens "critico" lonSen latSen
	send mens ant
end

if((tipo=="hola") && (ant == 999))
   set ant valor
   data mens tipo id
   send mens * valor
end

if(tipo=="alerta")
   send mens ant
end

areadsensor tempSen
rdata tempSen SensTipo idSens temp

if( temp>30)
   data mens "alerta" lonSen latSen
   send mens ant
end