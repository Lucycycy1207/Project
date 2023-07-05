/*The questions in this assignment are about doing soccer analytics using SQL.
The data is in tables England, France, Germany, and Italy.
These tables contain more than 100 years of soccer game statistics.
Follow the steps below to create your tables and familizarize yourself with the data.
Then write SQL statements to answer the questions.
The first time you access the data you might experience a small delay while DBeaver
loads the metadata for the tables accessed.

Submit this file after adding your queries.
Replace "Your query here" text with your query for each question.
Submit one spreadsheet file with your visualizations for questions 2, 7, 9, 10, 12
(one sheet per question named by question number, e.g. Q2, Q7, etc).
*/

/*Create the tables.*/

create table england as select * from bob.england;
create table france as select * from bob.france;
create table germany as select * from bob.germany;
create table italy as select * from bob.italy;

/*Familiarize yourself with the tables.*/
SELECT * FROM england;/*england"date", season, home, visitor, hgoal, vgoal, tier, totgoal, goaldif, result*/
SELECT * FROM germany;/*germany"date", season, home, visitor, hgoal, vgoal, tier*/
SELECT * FROM france;/*france"date", season, home, visitor, hgoal, vgoal, tier, totgoal, goaldif, result*/
SELECT * FROM italy;/*italy"date", season, home, visitor, hgoal, vgoal, tier*/

/*Q1 (1 pt)
Find all the games in England between seasons 1920 and 1999 such that the total goals are at least 13.
Order by total goals descending.*/

select * from england where season>1920 and season<1999 and totgoal>13 order by totgoal desc;

/*Sample result
1935-12-26,1935,Tranmere Rovers,Oldham Athletic,13,4,3,17,9,H
1958-10-11,1958,Tottenham Hotspur,Everton,10,4,1,14,6,H
...*/


/*Q2 (2 pt)
For each total goal result, find how many games had that result.
Use the england table and consider only the seasons since 1980.
Order by total goal.*/

select totgoal, count(totgoal) from england where season>=1980 group by totgoal order by totgoal;

/*Sample result
0,6085
1,14001
...*/

/*Visualize the results using a barchart.*/


/*Q3 (2 pt)
Find for each team in England in tier 1 the total number of games played since 1980.
Report only teams with at least 300 games.

Hint. Find the number of games each team has played as "home".
Find the number of games each team has played as "visitor".
Then union the two and take the sum of the number of games.
*/

with table1 as (select * from england where season>=1980 and tier=1)
select groupName, count1+count2 as totcount from (select * from (select visitor as groupName, count(visitor) as count1 from table1 group by visitor) as table2
    natural join
    (select home as groupName, count(home) as count2 from table1 group by home) as table3) as table4
 where count1+count2>=300 order by groupName
;

 /*Sample result
Everton,1451
Liverpool,1451
...*/

/*Q4 (1 pt)
For each pair team1, team2 in England, in tier 1,
find the number of home-wins since 1980 of team1 versus team2.
Order the results by the number of home-wins in descending order.

Hint. After selecting the tuples needed (... WHERE tier=1 AND ...) do a GROUP BY home, visitor.
*/
/*england"date", season, home, visitor, hgoal, vgoal, tier, totgoal, goaldif, result*/

with table1 as
(select *
from england
where season>=1980 and tier=1)
select home as team1, visitor as team2, count(result) as homeWins from table1 where result='H' group by team1, team2 order by homeWins desc;



/*Sample result
Manchester United,Tottenham Hotspur,27
Arsenal,Everton,26
...*/



/*Q5 (1 pt)
For each pair team1, team2 in England in tier 1
find the number of away-wins since 1980 of team1 versus team2.
Order the results by the number of away-wins in descending order.*/

with table1 as
(select *
from england
where season>=1980 and tier=1)
select visitor as team1, home as team2, count(result) as awayWins from table1 where result='A' group by team2, team1 order by awayWins desc;

/*Sample result
Manchester United,Aston Villa,18
Manchester United,Everton,17
...*/


/*Q6 (2 pt)
For each pair team1, team2 in England in tier 1 report the number of home-wins and away-wins
since 1980 of team1 versus team2.
Order the results by the number of away-wins in descending order.

Hint. Join the results of the two previous queries. To do that you can use those
queries as subqueries. Remove their ORDER BY clause when making them subqueries.
Be careful on the join conditions.
*/

with table1 as
(select *
from england
where season>=1980 and tier=1),
table2 as (select home as team1, visitor as team2, count(result) as homeWins from table1 where result='H' group by team1, team2),
table3 as (select visitor as team1, home as team2, count(result) as awayWins from table1 where result='A' group by team1, team2)
select * from table2 natural join table3 order by awayWins desc;


/*Sample result
Manchester United,Aston Villa,26,18
Arsenal,Aston Villa,20,17
...*/

--Create a view, called Wins, with the query for the previous question.
create view Wins as
with table1 as
(select *
from england
where season>=1980 and tier=1),
table2 as (select home as team1, visitor as team2, count(result) as homeWins from table1 where result='H' group by team1, team2),
table3 as (select visitor as team1, home as team2, count(result) as awayWins from table1 where result='A' group by team1, team2)
select * from table2 natural join table3 order by awayWins desc;

/*Q7 (2 pt)
For each pair ('Arsenal', team2), report the number of home-wins and away-wins
of Arsenal versus team2 and the number of home-wins and away-wins of team2 versus Arsenal
(all since 1980).
Order the results by the second number of away-wins in descending order.
Use view W1.*/
select * from (select team1, team2, homeWins as homeWins1, awayWins as awayWins1 from Wins where team1='Arsenal') as table1 natural join
(select team2 as team1, team1 as team2, homeWins as homeWins2, awayWins as awayWins2 from Wins where team2='Arsenal') as table2 order by awayWins2 desc;

/*Sample result
Arsenal,Liverpool,14,8,20,11
Arsenal,Manchester United,16,5,19,11
...*/

/*Drop view Wins.*/
DROP VIEW Wins;

/*Build two bar-charts, one visualizing the two home-wins columns, and the other visualizing the two away-wins columns.*/


/*Q8 (2 pt)
Winning at home is easier than winning as visitor.
Nevertheless, some teams have won more games as a visitor than when at home.
Find the team in Germany that has more away-wins than home-wins in total.
Print the team name, number of home-wins, and number of away-wins.*/

with table1 as (select home as team, count(home) as homeWins from germany where hgoal>vgoal group by team),
table2 as (select visitor as team, count(home) as awayWins from germany where hgoal<vgoal group by team)
select * from table1 natural join table2 where awayWins>homeWins;

/*Sample result
Wacker Burghausen	...	...*/


/*Q9 (3 pt)
One of the beliefs many people have about Italian soccer teams is that they play much more defense than offense.
Catenaccio or The Chain is a tactical system in football with a strong emphasis on defence.
In Italian, catenaccio means "door-bolt", which implies a highly organised and effective backline defence
focused on nullifying opponents' attacks and preventing goal-scoring opportunities.
In this question we would like to see whether the number of goals in Italy is on average smaller than in England.

Find the average total goals per season in England and Italy since the 1970 season.
The results should be (season, england_avg, italy_avg) triples, ordered by season.

Hint.
Subquery 1: Find the average total goals per season in England.
Subquery 2: Find the average total goals per season in Italy
   (there is no totgoal in table Italy. Take hgoal+vgoal).
Join the two subqueries on season.
*/

select * from (select * from (select season, avg(totgoal) as england_avg from england group by season) as Eng natural join
(select season, avg(hgoal+vgoal) as italy_avg from italy group by season) as italy) as tot where season>=1970 order by season;


--Build a line chart visualizing the results. What do you observe?

/*Sample result
1970,2.5290927021696252,2.1041666666666667
1971,2.5922090729783037,2.0125
...*/


/*Q10 (3 pt)
Find the number of games in France and England in tier 1 for each goal difference.
Return (goaldif, france_games, eng_games) triples, ordered by the goal difference.
Normalize the number of games returned dividing by the total number of games for the country in tier 1,
e.g. 1.0*COUNT(*)/(select count(*) from france where tier=1)  */

create view q10france as select goaldif,1.0*count(*)/(select count(*) from france where tier=1) as france_games from france where tier=1 group by goaldif;
create view q10eng as select goaldif,1.0*count(*)/(select count(*) from england where tier=1) as eng_games from england where tier=1 group by goaldif;
select * from q10france natural join q10eng order by goaldif;
drop view q10eng;
drop view q10france;

/*Sample result
-8,0.00011369234850494562,0.000062637018477920450987
-7,0.00011369234850494562,0.00010439503079653408
...*/

/*Visualize the results using a barchart.*/


/*Q11 (2 pt)
Find all the seasons when England had higher average total goals than France.
Consider only tier 1 for both countries.
Return (season,england_avg,france_avg) triples.
Order by season.*/

select * from (select * from (select season, avg(totgoal) as england_avg from england where tier=1 group by season) as Eng natural join
(select season, avg(totgoal) as france_avg from france where tier=1 group by season) as france) as tot where england_avg>france_avg order by season;

/*Sample result
1936,3.3658008658008658,3.3041666666666667
1952,3.2640692640692641,3.1437908496732026
...*/
