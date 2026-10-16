# PizuttiCAM

Programa de CAM para gerar **G-code de fresamento de placas de circuito impresso (PCB) em máquinas CNC**, empacotado para **Linux** em formato **Flatpak**.

O PizuttiCAM é um fork do [GERBER_X3](https://github.com/XRay3D/GERBER_X3), criado por Damir Bakiev (XRay3D). O projeto original é voltado ao Windows; este fork existe para oferecer o mesmo software no Linux, de um jeito fácil de instalar em qualquer distribuição.

> **Aviso importante:** este software gera código que comanda uma máquina CNC. Sempre revise o G-code gerado e faça uma simulação ou um teste a seco (sem material e sem fresa) antes de usar na máquina. O programa é fornecido sem garantia de nenhum tipo.

## Para que serve

Quem faz placas de circuito impresso em casa, na escola ou no laboratório costuma partir de arquivos gerados por programas de projeto eletrônico (como KiCad ou Eagle). O PizuttiCAM serve para transformar esses arquivos em instruções para a fresadora CNC:

- abrir os arquivos da placa (como Gerber, Excellon para furação e DXF);
- definir as ferramentas e os parâmetros de corte;
- gerar os percursos de isolação das trilhas, furação e recorte da placa;
- exportar o G-code para o controlador da sua CNC;
- exportar o desenho para PDF.

A lista completa de recursos e a forma de uso detalhada estão na documentação do [projeto original](https://github.com/XRay3D/GERBER_X3), que este fork acompanha.

## Sobre este fork

**Este projeto foi feito com "vibe coding".** As adaptações para o Linux (compilação, empacotamento e ajustes para rodar fora do Windows) foram desenvolvidas com a ajuda de inteligência artificial, sem revisão profissional de código. Por isso:

- podem existir bugs que não existem no projeto original;
- o código do programa em si, a lógica de CAM e os recursos pertencem ao projeto original, de XRay3D;
- este fork **não tem vínculo oficial** com o autor original.

**Sobre o nome.** PizuttiCAM é uma homenagem ao meu pai: usei o sobrenome que herdei dele para batizar o software.

## Instalação

A forma suportada de instalar é o **Flatpak**, que funciona na maioria das distribuições Linux e já traz as bibliotecas necessárias.

### Opção A: instalar o pacote pronto (recomendado)

1. Instale o Flatpak e adicione o repositório Flathub, se ainda não tiver. Veja as instruções para a sua distribuição em <https://flatpak.org/setup/>.
2. Baixe o arquivo `PizuttiCAM.flatpak` na página de [Releases](https://github.com/Cippzz/PizuttiCAM/releases).
3. Instale:

   ```bash
   flatpak install --user PizuttiCAM.flatpak
   ```

   O Flatpak vai baixar o runtime KDE (versão 6.11) do Flathub automaticamente, se ele ainda não estiver instalado.
4. Execute:

   ```bash
   flatpak run org.pizutticam.PizuttiCAM
   ```

   O programa também deve aparecer no menu de aplicativos.

### Opção B: compilar a partir do código-fonte

Esta opção demora. **Na primeira compilação, conte com algumas horas** (o projeto exige um compilador GCC muito recente, que o Flatpak compila junto). As compilações seguintes são bem mais rápidas, porque o `flatpak-builder` guarda o resultado dos módulos já prontos.

Recomendações: computador com vários núcleos, pelo menos 8 GB de memória RAM livres e vários GB de espaço em disco.

1. Instale o `flatpak` e o `flatpak-builder` pelo gerenciador de pacotes da sua distribuição. Por exemplo:

   ```bash
   # Debian, Ubuntu, Linux Mint
   sudo apt install flatpak flatpak-builder

   # Fedora
   sudo dnf install flatpak flatpak-builder
   ```

2. Adicione o Flathub e instale o runtime e o SDK do KDE:

   ```bash
   flatpak remote-add --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
   flatpak install flathub org.kde.Platform//6.11 org.kde.Sdk//6.11
   ```

3. Baixe o código e compile:

   ```bash
   git clone https://github.com/Cippzz/PizuttiCAM.git
   cd PizuttiCAM
   flatpak-builder --user --install --force-clean build-dir org.pizutticam.PizuttiCAM.yml
   ```

   A compilação precisa de acesso à internet para baixar as dependências.

4. Execute:

   ```bash
   flatpak run org.pizutticam.PizuttiCAM
   ```

### Gerar um pacote para instalar em outros computadores

Depois de compilar, é possível criar um arquivo `.flatpak` para levar para outras máquinas:

```bash
flatpak-builder --repo=repo --force-clean build-dir org.pizutticam.PizuttiCAM.yml
flatpak build-bundle repo PizuttiCAM.flatpak org.pizutticam.PizuttiCAM
```

Copie o `PizuttiCAM.flatpak` para o outro computador e siga a **Opção A**.

## Desinstalar

```bash
flatpak uninstall org.pizutticam.PizuttiCAM
```

## Compilar sem Flatpak (avançado)

Esta forma **não foi testada** por mim; o caminho testado é o Flatpak. Pelo `CMakeLists.txt` do projeto, os requisitos são:

- CMake 4.0 ou mais novo;
- GCC 16 ou mais novo (o código usa reflexão estática do C++26, com `-freflection`);
- Qt 6 (Widgets, Qml e LinguistTools);
- Boost 1.85 ou mais novo, com suporte a CMake;
- CGAL, GMP, MPFR e oneTBB;
- `simdjson` e `ctre`, que o CMake baixa sozinho.

Para um pacote que rode em outros computadores, use a opção `-DGERBER_NATIVE_MARCH=OFF`. Com ela ligada, que é o padrão do projeto, o executável usa instruções específicas do processador da máquina onde foi compilado e pode não funcionar em outras.

## Problemas e sugestões

Se algo não funcionar, abra uma [issue](https://github.com/Cippzz/PizuttiCAM/issues) informando sua distribuição Linux, a versão do Flatpak e a mensagem de erro. Para ver a saída do programa no terminal, rode `flatpak run org.pizutticam.PizuttiCAM` e copie o texto que aparecer.

## Créditos e licença

- Projeto original: [GERBER_X3](https://github.com/XRay3D/GERBER_X3), de Damir Bakiev (XRay3D), 2016–2026. Todo o mérito do programa de CAM é dele.
- O projeto original é distribuído sob a **Boost Software License 1.0**, e este fork mantém a mesma licença. Veja o arquivo `LICENSE`.
- As bibliotecas de terceiros usadas (Qt, Boost, CGAL, oneTBB, simdjson, ctre e outras) têm suas próprias licenças.
