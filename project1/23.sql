SELECT name From Trainer Where id in
(SELECT owner_id From CatchedPokemon Where level <= 10)
ORDER BY name;
