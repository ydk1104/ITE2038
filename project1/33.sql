SELECT SUM(level) From CatchedPokemon Where owner_id = (SELECT id From Trainer Where name = 'Matis');
