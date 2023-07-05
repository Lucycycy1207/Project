/*1*/
select name from Person where age <18;
/*2*/
SELECT pizzeria, pizza, price FROM (SELECT pizzeria, pizza, price FROM serves WHERE price < 10) as table1 NATURAL JOIN ( SELECT pizza FROM eats WHERE name = 'Amy' ) as table2;
/*3*/
select pizzeria, name, age from Person Natural join Frequents where age <18;
/*4*/
select pizzeria from Person Natural join Frequents where age <18 INTERSECT select pizzeria from Person Natural join Frequents where age >30;
/*5*/
with table1 as (select pizzeria, name as person1, age as age1 from Person Natural join Frequents where age <18),
table2 as (select pizzeria, name as person2, age as age2 from Person Natural join Frequents where age >30) select * from (table1 natural join table2);
/*6*/
select * from (select name, count(pizza) as count from Eats group by name)as table3 where count>1 order by count desc;
/*7*/
select pizza, avg(price) as avePrice from Serves group by pizza order by avePrice desc;
