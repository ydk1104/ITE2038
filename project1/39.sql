SELECT name From Trainer,
(SELECT owner_id From CatchedPokemon
GROUP BY pid, owner_id
HAVING COUNT(*) > 1) As Temp
Where id = owner_id
ORDER BY name;
