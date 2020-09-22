SELECT name From Trainer Where id in
(SELECT owner_id From CatchedPokemon 
GROUP BY pid, owner_id
HAVING COUNT(*) > 1)
ORDER BY name
