SELECT name From Pokemon, (SELECT pid From CatchedPokemon) As T Where id = pid
ORDER BY name
