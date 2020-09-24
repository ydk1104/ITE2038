SELECT owner_id, COUNT(owner_id) From CatchedPokemon
GROUP BY owner_id
Having COUNT(owner_id) = 
(SELECT MAX(cnt) From (SELECT COUNT(owner_id) AS cnt From CatchedPokemon
GROUP BY owner_id) AS T);
