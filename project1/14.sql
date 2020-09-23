SELECT name From Pokemon, (SELECT before_id From Evolution) As T
Where type = 'Grass' and id = before_id
