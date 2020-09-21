SELECT name,type From Pokemon Where id in (SELECT pid From CatchedPokemon Where level >= 30)
ORDER BY name
