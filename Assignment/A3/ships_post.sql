/*
This assignment introduces an example concerning World War II capital ships.
It involves the following relations:

Classes(class, type, country, numGuns, bore, displacement)
Ships(name, class, launched)  --launched is for year launched
Battles(name, date_fought)
Outcomes(ship, battle, result)

Ships are built in "classes" from the same design, and the class is usually
named for the first ship of that class.

Relation Classes records the name of the class,
the type (bb for battleship or bc for battlecruiser),
the country that built the ship, the number of main guns,
the bore (diameter of the gun barrel, in inches)
of the main guns, and the displacement (weight, in tons).

Relation Ships records the name of the ship, the name of its class,
and the year in which the ship was launched.

Relation Battles gives the name and date of battles involving these ships.

Relation Outcomes gives the result (sunk, damaged, or ok)
for each ship in each battle.
*/

/*
Exercise 1. (1 point)

1.	Create simple SQL statements to create the above relations
    (no constraints for initial creations).
2.	Insert the following data.

For Classes:class, type, country, numGuns, bore, displacement
('Bismarck','bb','Germany',8,15,42000);
('Kongo','bc','Japan',8,14,32000);
('North Carolina','bb','USA',9,16,37000);
('Renown','bc','Gt. Britain',6,15,32000);
('Revenge','bb','Gt. Britain',8,15,29000);
('Tennessee','bb','USA',12,14,32000);
('Yamato','bb','Japan',9,18,65000);

For Ships:name, class, launched
('California','Tennessee',1921);
('Haruna','Kongo',1915);
('Hiei','Kongo',1914);
('Iowa','Iowa',1943);
('Kirishima','Kongo',1914);
('Kongo','Kongo',1913);
('Missouri','Iowa',1944);
('Musashi','Yamato',1942);
('New Jersey','Iowa',1943);
('North Carolina','North Carolina',1941);
('Ramilles','Revenge',1917);
('Renown','Renown',1916);
('Repulse','Renown',1916);
('Resolution','Revenge',1916);
('Revenge','Revenge',1916);
('Royal Oak','Revenge',1916);
('Royal Sovereign','Revenge',1916);
('Tennessee','Tennessee',1920);
('Washington','North Carolina',1941);
('Wisconsin','Iowa',1944);
('Yamato','Yamato',1941);

For Battles
('North Atlantic','27-May-1941');
('Guadalcanal','15-Nov-1942');
('North Cape','26-Dec-1943');
('Surigao Strait','25-Oct-1944');

For Outcomes
('Bismarck','North Atlantic', 'sunk');
('California','Surigao Strait', 'ok');
('Duke of York','North Cape', 'ok');
('Fuso','Surigao Strait', 'sunk');
('Hood','North Atlantic', 'sunk');
('King George V','North Atlantic', 'ok');
('Kirishima','Guadalcanal', 'sunk');
('Prince of Wales','North Atlantic', 'damaged');
('Rodney','North Atlantic', 'ok');
('Scharnhorst','North Cape', 'sunk');
('South Dakota','Guadalcanal', 'ok');
('West Virginia','Surigao Strait', 'ok');
('Yamashiro','Surigao Strait', 'sunk');
*/

create table Classes(
    class varchar(30),
    type varchar(10),
    country varchar(20),
    numGuns int,
    bore int,
    displacement int
);

create table Ships(
    name varchar(20),
    class varchar(30),
    launched int
);

create table Battles(
    name varchar(20),
    date_fought date
);

create table Outcomes(
    ship varchar(20),
    battle varchar(20),
    result varchar(7)
);

--for Classes
insert into Classes values('Bismarck','bb','Germany',8,15,42000);
insert into Classes values('Kongo','bc','Japan',8,14,32000);
insert into Classes values('North Carolina','bb','USA',9,16,37000);
insert into Classes values('Renown','bc','Gt. Britain',6,15,32000);
insert into Classes values('Revenge','bb','Gt. Britain',8,15,29000);
insert into Classes values('Tennessee','bb','USA',12,14,32000);
insert into Classes values('Yamato','bb','Japan',9,18,65000);

--for Ships
insert into Ships values('California','Tennessee',1921);
insert into Ships values('Haruna','Kongo',1915);
insert into Ships values('Hiei','Kongo',1914);
insert into Ships values('Iowa','Iowa',1943);
insert into Ships values('Kirishima','Kongo',1914);
insert into Ships values('Kongo','Kongo',1913);
insert into Ships values('Missouri','Iowa',1944);
insert into Ships values('Musashi','Yamato',1942);
insert into Ships values('New Jersey','Iowa',1943);
insert into Ships values('North Carolina','North Carolina',1941);
insert into Ships values('Ramilles','Revenge',1917);
insert into Ships values('Renown','Renown',1916);
insert into Ships values('Repulse','Renown',1916);
insert into Ships values('Resolution','Revenge',1916);
insert into Ships values('Revenge','Revenge',1916);
insert into Ships values('Royal Oak','Revenge',1916);
insert into Ships values('Royal Sovereign','Revenge',1916);
insert into Ships values('Tennessee','Tennessee',1920);
insert into Ships values('Washington','North Carolina',1941);
insert into Ships values('Wisconsin','Iowa',1944);
insert into Ships values('Yamato','Yamato',1941);

--For Battles
insert into Battles values('North Atlantic','27-May-1941');
insert into Battles values('Guadalcanal','15-Nov-1942');
insert into Battles values('North Cape','26-Dec-1943');
insert into Battles values('Surigao Strait','25-Oct-1944');

--For Outcomes
insert into Outcomes values('Bismarck','North Atlantic', 'sunk');
insert into Outcomes values('California','Surigao Strait', 'ok');
insert into Outcomes values('Duke of York','North Cape', 'ok');
insert into Outcomes values('Fuso','Surigao Strait', 'sunk');
insert into Outcomes values('Hood','North Atlantic', 'sunk');
insert into Outcomes values('King George V','North Atlantic', 'ok');
insert into Outcomes values('Kirishima','Guadalcanal', 'sunk');
insert into Outcomes values('Prince of Wales','North Atlantic', 'damaged');
insert into Outcomes values('Rodney','North Atlantic', 'ok');
insert into Outcomes values('Scharnhorst','North Cape', 'sunk');
insert into Outcomes values('South Dakota','Guadalcanal', 'ok');
insert into Outcomes values('West Virginia','Surigao Strait', 'ok');
insert into Outcomes values('Yamashiro','Surigao Strait', 'sunk');

select * from Classes;
select * from Ships;
select * from Battles;
select * from Outcomes;

-- Exercise 2. (6 points)
-- Write SQL queries for the following requirements.

-- 1.	(2 pts) List the name, displacement, and number of guns of the ships engaged in the battle of Guadalcanal.
/*
Expected result:
ship,displacement,numguns
Kirishima,32000,8
South Dakota,NULL,NULL
*/
create view t1 as
(select ship from Outcomes where battle = 'Guadalcanal');

create view t2 as(
select * from t1 left join Ships on t1.ship=ships.name);

create view t3 as (
select t2.class from t2 left join Classes on t2.class=Classes.class);
select ship, displacement, numguns from t2 left join Classes on t2.class=Classes.class;

drop view t3;
drop view t2;
drop view t1;


-- 2.	(2 pts) Find the names of the ships whose number of guns was the largest for those ships of the same bore.
create view t1 as (
select name, class, numGuns, bore from ships natural join Classes);
create view t2 as (
select max(numGuns) as numGuns, bore from t1 group by bore);

select name,class,numGuns, bore from t1 natural join t2 order by bore;

drop view t2;
drop view t1;

--3. (2 pts) Find for each class with at least three ships the number of ships of that class sunk in battle.
/*
class,sunk_ships
Revenge,0
Kongo,1
Iowa,0
*/
create view t1 as (
select count(name) as ship_num, class from Ships group by class);
select class from t1 where ship_num>=3;

create view t2 as (--class Revenge Kongo Iowa
select name, class from Ships
where class in
      (select class from t1 where ship_num>=3));
create view t3 as (
select name, class, result from t2 left join Outcomes on t2.name=Outcomes.ship);
create view t4 as (
select
       class,
       case
           when result='sunk' then 1
           else 0
       end as sunk_ships
from t3);
select class, sum(sunk_ships) from t4 group by class;

drop view t4;
drop view t3;
drop view t2;
drop view t1;

-- Exercise 3. (4 points)

-- Write the following modifications.

-- 1.	(2 points) Two of the three battleships of the Italian Vittorio Veneto class –
-- Vittorio Veneto and Italia – were launched in 1940;
-- the third ship of that class, Roma, was launched in 1942.
-- Each had 15-inch guns and a displacement of 41,000 tons.
-- Insert these facts into the database.

insert into Ships values('Vittorio Veneto', 'Vittorio Veneto', 1940);
insert into Ships values('Italia', 'Vittorio Veneto', 1940);
insert into Ships values('Roma', 'Vittorio Veneto', 1942);

insert into Classes values('Italian Vittorio Veneto', 'bb', 'Italian', 15, NULL, 41000);

-- 2.	(1 point) Delete all classes with fewer than three ships.

create view t1 as (
select count(name) as count, class from Ships group by class);
-----------------------------------------------------------------delete from Ships where class in (select class from t1 where count<3);
delete from Classes where class in (select class from t1 where count<3);
drop view t1;

-- 3.	(1 point) Modify the Classes relation so that gun bores are measured in centimeters
-- (one inch = 2.5 cm) and displacements are measured in metric tons (one metric ton = 1.1 ton).

update Classes
    set bore=bore*2.5, displacement=displacement/1.1;

-- Exercise 4.  (9 points)
-- Add the following constraints using views with check option.

--1. (3 points) No ship can be in battle before it is launched.

CREATE OR REPLACE VIEW OutcomesView AS
SELECT ship, battle, result
FROM Outcomes O
WHERE NOT EXISTS (
    SELECT *
    FROM Ships S, Battles B
    WHERE S.name=O.ship AND O.battle=B.name AND
    S.launched > extract(year from B.date_fought)
)
WITH CHECK OPTION;

-- Now we can try some insertion on this view.
INSERT INTO OutcomesView (ship, battle, result)
VALUES('Musashi', 'North Atlantic','ok');
-- This insertion, as expected, should fail since Musashi is launched in 1942,
-- while the North Atlantic battle took place on 27-MAY-41.


-- 2. (3 points) No ship can be launched before
-- the ship that bears the name of the first ship’s class.

CREATE OR REPLACE VIEW ShipsV AS
SELECT name, class, launched
FROM Ships S
WHERE NOT EXISTS (
    SELECT *
    FROM Ships S1
    WHERE S1.class = S.class and S1.class=S1.name and S1.launched > S.launched
    )
WITH CHECK OPTION;

-- Now we can try some insertion on this view.
INSERT INTO ShipsV(name, class, launched)
VALUES ('AAA','Kongo',1912);
-- This insertion, as expected, should fail since ship Kongo (first ship of class Kongo) is launched in 1913.

--3. (3 points) No ship fought in a battle that was at a later date than another battle in which that ship was sunk.

CREATE OR REPLACE VIEW OutcomesV AS
SELECT ship, battle, result
FROM Outcomes O
WHERE NOT EXISTS (
    SELECT *
    FROM Battles B,Battles B1, Outcomes O1
    WHERE O.ship = O1.ship and O1.result='sunk'
                and B.name=O.battle and B1.name=O1.battle and B.date_fought>B1.date_fought
)
WITH CHECK OPTION;

-- Now we can try some insertion on this view.
INSERT INTO OutcomesV(ship, battle, result)
VALUES('Bismarck', 'Guadalcanal', 'ok');
-- This insertion, as expected, should fail since 'Bismarck' was sunk in
-- the battle of North Atlantic, in 1941, whereas the battle of Guadalcanal happened in 1942.