# Relatório de Sugestões para o CLI GSDENSITY

## Introdução
Este relatório apresenta uma análise comparativa entre a proposta inicial do CLI `gsdensity` do usuário e o projeto `grleconvert` existente no GitHub, com o objetivo de fornecer sugestões de melhoria para o `gsdensity`, focando na compatibilidade e funcionalidade para o Farming Simulator 20 (FS20). O `grleconvert` é uma ferramenta robusta para conversão de arquivos GDM (Giants Density Map) e GRLE (Giants Run-Length Encoded) para e de PNG, embora seu foco principal seja nas versões mais recentes do Farming Simulator (FS22/FS25) [1].

## Análise do Projeto `grleconvert`
O projeto `grleconvert` (escrito em Rust) demonstra uma implementação eficaz para lidar com os formatos GDM e GRLE. As principais características e aprendizados incluem:

### 1. Descoberta de Parâmetros via Arquivo I3D
Uma funcionalidade crucial do `grleconvert` é a capacidade de **detectar automaticamente os parâmetros de codificação** (como número de canais e tipo de camada) a partir do arquivo `.i3d` do mapa. Ele faz isso buscando o `.i3d` na hierarquia de diretórios a partir do arquivo de entrada. Isso simplifica significativamente a experiência do usuário, eliminando a necessidade de especificar manualmente muitos parâmetros [1].

### 2. Tratamento de Formatos GDM e GRLE
O `grleconvert` lida com as especificidades de ambos os formatos:
- **GRLE**: Utilizado para camadas de informação (ex: farmlands, field types), sempre em escala de cinza (1 canal) e comprimido via RLE [2].
- **GDM**: Utilizado para mapas de densidade (ex: altura, folhagem), podendo ter de 1 a 12 canais (ou mais, dependendo da versão do jogo) e utilizando compressão baseada em blocos e paletas. O formato GDM pode ter diferentes variantes de cabeçalho (`"MDF` ou `!MDF`) e múltiplos *ranges* de compressão, especialmente para mapas de altura [3].

### 3. Subcomando `pixel_guide`
Além da conversão, o `grleconvert` inclui um utilitário `pixel_guide` que gera um documento Markdown detalhando o significado de cada valor de pixel para as camadas de densidade e informação de um mapa. Isso é inestimável para modders que precisam entender como editar os mapas corretamente [1].

### 4. Estrutura de Comando
O `grleconvert` adota uma abordagem de comando único com opções para especificar o comportamento (codificar/decodificar é inferido pela extensão do arquivo). No entanto, ele também oferece subcomandos para utilitários adicionais (`compare_pngs`, `pixel_guide`) [1].

## Análise do CLI `gsdensity` Proposto
A proposta do `gsdensity` é clara e modular, utilizando subcomandos `encoder`, `decoder` e `verify`. Isso é uma boa prática para CLIs, pois organiza as funcionalidades de forma lógica.

### Estrutura Proposta:
```
gsdensity
-v, --version
-h, --help

gsdensity encoder
-i, --i3d # optional i3d map
-l, --layers # reuses layers from a gdm/grle file and adds layers to the new gdm/grle file.
-n, --num-channels # Automatic if specified --i3d, retrieves directly from the map's i3d file.
-o, --output (gdm, grle)

gsdensity decoder
-f, --file (*.grle, *.gdm)
-o, --output-dir

gsdensity verify
-f, --file (*.grle, *.gdm)
-n, --num-channels
```

## Sugestões de Melhoria para o `gsdensity` (Foco em FS20)
Com base na análise do `grleconvert` e nas especificidades do FS20, as seguintes melhorias são sugeridas para o `gsdensity`:

### 1. Unificação e Flexibilidade dos Comandos de Conversão
Embora a separação `encoder`/`decoder` seja clara, considere uma abordagem mais unificada, similar ao `grleconvert`, onde a ação (codificar/decodificar) é inferida pela extensão do arquivo de entrada/saída. Isso pode simplificar o uso para operações comuns. No entanto, manter os subcomandos explícitos para `encoder` e `decoder` também é válido e pode ser mais intuitivo para alguns usuários.

**Recomendação:** Manter os subcomandos `encoder` e `decoder` como estão, mas garantir que a detecção automática de formato seja robusta.

### 2. Descoberta Automática de Parâmetros I3D
O `grleconvert` implementa uma busca recursiva pelo arquivo `.i3d` na hierarquia de diretórios. Esta é uma funcionalidade essencial para o FS20, pois os arquivos `.i3d` contêm metadados cruciais para a correta interpretação e codificação dos mapas de densidade.

**Sugestão:** Implementar a lógica de `find_i3d_file` do `grleconvert` para que o `gsdensity encoder` possa automaticamente descobrir o `.i3d` e extrair `LayerParams` (tipo de camada, número de canais, canais de compressão) [1]. A opção `-i, --i3d` deve permanecer para permitir a especificação manual quando a detecção automática falhar ou for indesejada.

### 3. Tratamento Robusto de Canais e Compressão GDM
O FS20, assim como versões posteriores, utiliza GDM com múltiplos canais e, em alguns casos, com *compression channels* (canais de compressão) para dividir os dados em *ranges* para compressão. O `grleconvert` lida com isso através das opções `--channels` e `--compress-at` [1].

**Sugestão para `gsdensity encoder`:**
- A opção `-n, --num-channels` deve ser capaz de ser inferida do `.i3d`. Se não for fornecida e o `.i3d` não for encontrado, deve ser um parâmetro obrigatório para GDM.
- Adicionar uma opção para especificar os canais de compressão, por exemplo, `--compression-split <num>`, para GDM. Isso é crucial para a correta codificação de certos tipos de mapas de densidade (ex: `height_map.gdm`).

### 4. Reutilização e Expansão de Camadas (`--layers`)
A opção `-l, --layers` na sua proposta para o `encoder` é uma funcionalidade poderosa para a edição de mapas. Ela permite combinar camadas PNG existentes para formar um novo arquivo GDM/GRLE, com a capacidade de expandir o número total de canais.

**Sugestão:** A opção `--layers` deve aceitar uma lista de arquivos PNG de entrada. Esses arquivos PNG serão interpretados como as camadas iniciais do novo arquivo GDM/GRLE. Se o número total de canais especificado por `-n, --num-channels` for maior do que o número de arquivos PNG fornecidos, as camadas adicionais devem ser preenchidas automaticamente com dados de valor zero (equivalente a "preto" ou vazio), permitindo a expansão da estrutura do mapa sem a necessidade de conteúdo inicial para os novos canais. Isso é particularmente útil para mapas que requerem um número maior de canais para futuras edições, mantendo a compatibilidade com os dados originais.

**Exemplo de Uso:**
`gsdensity encoder --layers l01.png l02.png l03.png l04.png l05.png l06.png --num-channels 12 --output sample_cultivator_density.gdm`
Neste exemplo, `l01.png` a `l06.png` seriam as seis primeiras camadas, e as seis camadas restantes seriam preenchidas com zeros, resultando em um GDM de 12 canais.

### 5. Melhorias no Subcomando `verify`
O subcomando `verify` é uma excelente adição. Para torná-lo mais poderoso e específico para o FS20:

**Sugestão para `gsdensity verify`:**
- **Verificação de Cabeçalho**: Validar a mágica (`GRLE`, `"MDF`, `!MDF`), versão e dimensões do arquivo. Para GDM, verificar a variante do cabeçalho (`"MDF` vs `!MDF`) que pode indicar diferenças de formato entre as versões do jogo [3].
- **Comparação com I3D**: Se um arquivo `.i3d` for fornecido (ou descoberto automaticamente), o `verify` pode comparar os parâmetros do arquivo GDM/GRLE (dimensões, número de canais) com o que está declarado no `.i3d`, identificando inconsistências.
- **Verificação de Integridade de Dados**: Embora complexo, uma verificação básica da estrutura dos blocos de dados (para GDM) ou da sequência RLE (para GRLE) pode ser considerada para detectar corrupção básica.
- **Saída Detalhada**: Fornecer uma saída clara e legível sobre o que foi verificado e quaisquer problemas encontrados.

### 6. Adição de um Subcomando `inspect` (Inspirado em `pixel_guide`)
O `pixel_guide` do `grleconvert` é uma ferramenta valiosa para entender os dados do mapa. Um subcomando similar no `gsdensity` seria extremamente útil para a comunidade de modding do FS20.

**Sugestão:** Adicionar um subcomando `gsdensity inspect` (ou `gsdensity guide`) que:
- Aceite um arquivo `.i3d` como entrada.
- Analise as camadas de densidade e informação referenciadas no `.i3d`.
- Gere um relatório (em Markdown ou texto simples) explicando o significado dos canais e valores de pixel para cada camada (ex: quais bits representam o tipo de fruta, qual o estágio de crescimento, etc.).
- Poderia opcionalmente aceitar um `--data-dir` para carregar arquivos de configuração do jogo (ex: `fruitTypes.xml`, `farmlands.xml`) para fornecer descrições mais ricas [1].

### 7. Modo Batch/Diretório
Modders frequentemente precisam processar múltiplos arquivos. Um modo que permita processar todos os arquivos `.gdm` ou `.grle` em um diretório (e seus subdiretórios) seria muito eficiente.

**Sugestão:** Adicionar uma opção global ou um subcomando específico (ex: `gsdensity batch encode <dir>`) que itere sobre os arquivos e aplique a operação de conversão.

## Resumo das Recomendações para o CLI `gsdensity`

| Comando/Opção | Descrição | Justificativa | Exemplo de Uso | Status Atual (Proposta) | Recomendação | Prioridade |
|---|---|---|---|---|---|---|
| `gsdensity encoder` | Codifica PNG para GDM/GRLE | Funcionalidade central | `gsdensity encoder input.png -o output.gdm` | OK | Manter, com melhorias de inferência | Alta |
| `-i, --i3d` | Caminho para o arquivo `.i3d` | Essencial para descoberta de parâmetros | `gsdensity encoder input.png -o output.gdm --i3d map.i3d` | OK | Manter, mas tornar opcional com busca automática | Alta |
| `-l, --layers` | Combina e expande camadas PNG para novo GDM/GRLE | Permite reutilizar dados existentes e expandir canais | `gsdensity encoder --layers l01.png ... l06.png -n 12 -o output.gdm` | OK | Manter, com preenchimento automático de canais vazios | Alta |
| `-n, --num-channels` | Número de canais | Parâmetro crucial para GDM | `gsdensity encoder input.png -o output.gdm --num-channels 10` | OK | Manter, inferir do i3d se possível, obrigatório para GDM sem i3d | Alta |
| `--compression-split <num>` | Ponto de divisão para canais de compressão GDM | Necessário para GDM complexos (ex: height maps) | `gsdensity encoder input.png -o output.gdm --num-channels 12 --compression-split 8` | Novo | Adicionar | Alta |
| `gsdensity decoder` | Decodifica GDM/GRLE para PNG | Funcionalidade central | `gsdensity decoder -f input.gdm -o output_dir` | OK | Manter | Alta |
| `gsdensity verify` | Verifica a integridade do arquivo | Garante a validade dos arquivos | `gsdensity verify -f input.gdm` | OK | Expandir com verificação de cabeçalho e i3d | Alta |
| `gsdensity inspect` | Gera guia de valores de pixel | Ferramenta de apoio para modders | `gsdensity inspect map.i3d` | Novo | Adicionar | Alta |
| `gsdensity batch` | Processa múltiplos arquivos em um diretório | Eficiência para modders | `gsdensity batch encode /path/to/pngs /path/to/gdms` | Novo | Adicionar | Média |

## Conclusão
O CLI `gsdensity` tem uma base sólida. Ao incorporar as melhores práticas e funcionalidades observadas no `grleconvert`, especialmente a descoberta automática de parâmetros via `.i3d` e o tratamento robusto das especificidades do formato GDM (como canais de compressão), além de adicionar utilitários como o `inspect` e o modo `batch`, a ferramenta se tornará muito mais poderosa e amigável para a comunidade de modding do Farming Simulator 20. A atenção aos detalhes do formato FS20, como as variantes de cabeçalho GDM, garantirá a compatibilidade e a robustez da ferramenta.

## Referências
[1] Paint-a-Farm. (n.d.). *Paint-a-Farm/grleconvert: Cross-platform tool for converting GIANTS Engine density map files (GRLE and GDM) to and from PNG images*. GitHub. Disponível em: [https://github.com/Paint-a-Farm/grleconvert](https://github.com/Paint-a-Farm/grleconvert)
[2] Paint-a-Farm. (n.d.). *GRLE (Giants Run-Length Encoded) File Format Specification*. GitHub. Disponível em: [https://github.com/Paint-a-Farm/grleconvert/blob/master/docs/GRLE_FORMAT.md](https://github.com/Paint-a-Farm/grleconvert/blob/master/docs/GRLE_FORMAT.md)
[3] Paint-a-Farm. (n.d.). *GDM (Giants Density Map) File Format Specification*. GitHub. Disponível em: [https://github.com/Paint-a-Farm/grleconvert/blob/master/docs/GDM_FORMAT.md](https://github.com/Paint-a-Farm/grleconvert/blob/master/docs/GDM_FORMAT.md)
