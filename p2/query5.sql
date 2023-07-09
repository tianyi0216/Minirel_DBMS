WITH sellers AS(
    SELECT COUNT(*)
    FROM Users S, Auctions A
    WHERE S.id = A.id AND S.rating > 1000
    GROUP BY S.id
)
SELECT COUNT(*)
FROM sellers;