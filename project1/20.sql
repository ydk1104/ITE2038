SELECT name, COUNT(*) as cnt From CatchedPokemon, Trainer Where
CatchedPokemon.owner_id = Trainer.id and
owner_id in
(SELECT id From Trainer Where hometown = 'Sangnok City')
GROUP BY owner_id
ORDER BY cnt;
