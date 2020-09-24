SELECT hometown, nickname From(
SELECT hometown, MAX(max_level) as max_level_town From(
SELECT owner_id, MAX(level) as max_level From CatchedPokemon
GROUP BY owner_id) As Temp RIGHT JOIN Trainer
ON owner_id = id
GROUP BY hometown) As Temp LEFT JOIN CatchedPokemon
ON max_level_town = level and hometown = (SELECT hometown From Trainer Where id=owner_id)
ORDER BY hometown;
