SELECT COUNT(DISTINCT pid) From CatchedPokemon Where
owner_id in (SELECT id From Trainer Where hometown = 'Sangnok City');
