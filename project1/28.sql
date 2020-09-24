SELECT name, avg_level From
(SELECT owner_id, AVG(level) as avg_level From CatchedPokemon Where pid in
(SELECT id From Pokemon Where type = 'Normal' or type = 'Electric')
GROUP BY owner_id) As temp, Trainer
Where owner_id = Trainer.id
ORDER BY avg_level;
